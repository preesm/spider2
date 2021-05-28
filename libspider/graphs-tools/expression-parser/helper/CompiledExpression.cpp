/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
 * applications. The Preesm tool may be used to design PiSDF applications.
 *
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */
#if defined(__linux__) && defined(_SPIDER_JIT_EXPRESSION)

/* === Include(s) === */

#include <graphs-tools/expression-parser/helper/CompiledExpression.h>
#include <graphs/pisdf/Param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <spawn.h>
#include <dlfcn.h>

/* === Function(s) definition === */

void *spider::expr::CompiledExpression::hndl_{ };
extern char **environ;

spider::expr::CompiledExpression::CompiledExpression(const spider::vector<RPNElement> &postfixStack,
                                                     const param_table_t &params) {
    /* == Tries to create the folder if it does not already exists == */
    if (mkdir("./.cache", 0755) < 0 && errno != EEXIST) {
        throwSpiderException("failed to create directory for jit compiled expressions.");
    }

    /* == Write helper functions (only once) == */
    writeHelperFile();
    /* == Convert string to C++ syntax == */
    const auto stack = convertToCpp(postfixStack);
    /* == Compute hash for equality == */
    hash_ = std::hash<std::string>{ }(rpn::postfixString(stack));
    /* == Perform partial evaluation of the expression (if possible) and compile the expression stack == */
    compile(stack, params);
}

double spider::expr::CompiledExpression::evaluate(const param_table_t &params) {
    /* == check if expression has been imported == */
    if (!hndl_) {
        const auto func = std::string("expr_").append(std::to_string(hash_));
        /* == Invoke g++ to compile expression == */
        compileExpression();
        /* == Import function == */
        expr_ = importExpression(func);
    } else if (hndl_ != localHndlCpy_) {
        const auto func = std::string("expr_").append(std::to_string(hash_));
        /* == Import function == */
        expr_ = importExpression(func);
    }
    updateSymbolTable(params);
    return expr_(valueTable_.data());
}

/* === Private method(s) === */

spider::vector<RPNElement>
spider::expr::CompiledExpression::convertToCpp(const spider::vector<RPNElement> &postfixStack) {
    auto res = factory::vector<RPNElement>(postfixStack, StackID::EXPRESSION);
    for (auto &e : res) {
        if (e.token_ == "^") {
            e.token_ = "jitexpr::pow";
            e.subtype_ = RPNElementSubType::FUNCTION;
        } else if (e.token_ == "and") {
            e.token_ = "jitexpr::land";
        } else if (e.token_ == "or") {
            e.token_ = "jitexpr::lor";
        } else if (e.token_ == "if") {
            e.token_ = "jitexpr::ifelse";
        }
    }
    return res;
}

spider::expr::CompiledExpression::param_t
spider::expr::CompiledExpression::findParameter(const param_table_t &params, const std::string &name) {
    for (const auto &p : params) {
        if (p->name() == name) {
            return p.get();
        }
    }
    throwSpiderException("Did not find parameter [%s] for expression parsing.", name.c_str());
}

void spider::expr::CompiledExpression::registerSymbol(const param_t param) {
    for (const auto &s : symbolTable_) {
        if (s.second == param->name()) {
            return;
        }
    }
    symbolTable_.emplace_back(param->ix(), param->name());
    valueTable_.emplace_back(0.);
}

void spider::expr::CompiledExpression::updateSymbolTable(const param_table_t &params) {
#ifndef NDEBUG
    auto it = valueTable_.begin();
    for (const auto &sym : symbolTable_) {
        bool found = false;
        for (const auto &p : params) {
            if (sym.second == p->name()) {
                *(it++) = static_cast<double>(p->value(params));
                found = true;
                break;
            }
        }
        if (!found) {
            throwSpiderException("missing parameter [%s] for expression evaluation.", sym.second.c_str());
        }
    }
#else
    auto it = valueTable_.begin();
    for (const auto &sym : symbolTable_) {
        *(it++) = static_cast<double>(params[sym.first]->value(params));
    }
#endif
}

void spider::expr::CompiledExpression::compile(const vector<RPNElement> &postfixStack, const param_table_t &params) {
    /* == Register params == */
    for (const auto &e : postfixStack) {
        if (e.subtype_ == RPNElementSubType::PARAMETER) {
            const auto param = findParameter(params, e.token_);
            registerSymbol(param);
        }
    }
    const auto func = std::string("expr_") + std::to_string(hash_);
    /* == Create cpp file == */
    writeFunctionFile(func, rpn::infixString(postfixStack), symbolTable_);
    if (hndl_) {
        /* == we need to invalidate current lib == */
        dlclose(hndl_);
        hndl_ = nullptr;
        /* == Invoke g++ to compile expression == */
        compileExpression();
        /* == Import function == */
        expr_ = importExpression(func);
    }
}

void spider::expr::CompiledExpression::writeFunctionFile(const std::string &func,
                                                         const std::string &expression,
                                                         const spider::vector<std::pair<size_t, std::string>> &args) {
    /* == Check if file already exists == */
    FILE *outputFile;
    if (FILE *file = fopen("./.cache/libjitexpr.cpp", "r+")) {
        outputFile = file;
    } else {
        outputFile = fopen("./.cache/libjitexpr.cpp", "w+");
        if (!outputFile) {
            throwNullptrException();
        }
        printer::fprintf(outputFile, "#include \"jitexpr-helper.h\"\n\n");
        printer::fprintf(outputFile, "extern \"C\" {\n");
    }
    /* == check if the function was already written == */
    char tmp[512];
    while (fgets(tmp, 512, outputFile)) {
        if (strstr(tmp, func.c_str())) {
            fclose(outputFile);
            return;
        }
    }
    fseek(outputFile, -1, SEEK_END);
    printer::fprintf(outputFile, "\n\tdouble %s(const double *args) {\n", func.c_str());
    printer::fprintf(outputFile, "\t\tusing namespace std;\n");
    for (size_t i = 0; i < args.size(); ++i) {
        printer::fprintf(outputFile, "\t\tconst auto %s = args[%zuu];\n", args[i].second.c_str(), i);
    }
    printer::fprintf(outputFile, "\t\treturn %s;\n", expression.c_str());
    printer::fprintf(outputFile, "\t}\n");
    printer::fprintf(outputFile, "}"); /* = this finalize the extern 'C' = */
    fclose(outputFile);
}

void spider::expr::CompiledExpression::writeHelperFile() {
    const auto fileName = "./.cache/jitexpr-helper.h";
    if (FILE *file = fopen(fileName, "r")) {
        fclose(file);
        return;
    }
    FILE *outputFile = fopen(fileName, "w+");
    if (outputFile) {
        printer::fprintf(outputFile, "#ifndef JITEXPR_HELPER_FCT_H\n");
        printer::fprintf(outputFile, "#define JITEXPR_HELPER_FCT_H\n\n");
        printer::fprintf(outputFile, "#include <cmath>\n");
        printer::fprintf(outputFile, "#include <functional>\n\n");
        printer::fprintf(outputFile, "namespace jitexpr {\n");
        /* == Conditional if == */
        printer::fprintf(outputFile, "\tinline double ifelse(bool p, const double b0, const double b1) {\n");
        printer::fprintf(outputFile, "\t\tif(p) {\n");
        printer::fprintf(outputFile, "\t\t\treturn b0;\n");
        printer::fprintf(outputFile, "\t\t}\n");
        printer::fprintf(outputFile, "\t\treturn b1;\n");
        printer::fprintf(outputFile, "\t}\n\n");
        /* == Logical AND == */
        printer::fprintf(outputFile, "\tinline double land(const double x, const double y) {\n");
        printer::fprintf(outputFile, "\t\tif(std::not_equal_to<double>{ }(0., x) && \n"
                                     "\t\t   std::not_equal_to<double>{ }(0., y)) {\n");
        printer::fprintf(outputFile, "\t\t\treturn 1.;\n");
        printer::fprintf(outputFile, "\t\t}\n");
        printer::fprintf(outputFile, "\t\treturn 0.;\n");
        printer::fprintf(outputFile, "\t}\n\n");
        /* == Logical OR == */
        printer::fprintf(outputFile, "\tinline double lor(const double x, const double y) {\n");
        printer::fprintf(outputFile, "\t\tif(std::not_equal_to<double>{ }(0., x) || \n"
                                     "\t\t   std::not_equal_to<double>{ }(0., y)) {\n");
        printer::fprintf(outputFile, "\t\t\treturn 1.;\n");
        printer::fprintf(outputFile, "\t\t}\n");
        printer::fprintf(outputFile, "\t\treturn 0.;\n");
        printer::fprintf(outputFile, "\t}\n\n");
        /* == pow optimized function (see: https://baptiste-wicht.com/posts/2017/09/cpp11-performance-tip-when-to-use-std-pow.html) == */
        printer::fprintf(outputFile, "\tinline double pow(const double x, int n) {\n");
        printer::fprintf(outputFile, "\t\tif(n < 100) {\n");
        printer::fprintf(outputFile, "\t\t\tauto r { x };\n");
        printer::fprintf(outputFile, "\t\t\twhile(n > 1) {\n");
        printer::fprintf(outputFile, "\t\t\t\tr *= x;\n");
        printer::fprintf(outputFile, "\t\t\t\tn -= 1;\n");
        printer::fprintf(outputFile, "\t\t\t}\n");
        printer::fprintf(outputFile, "\t\t\treturn r;\n");
        printer::fprintf(outputFile, "\t\t}\n");
        printer::fprintf(outputFile, "\t\treturn std::pow(x, n);\n");
        printer::fprintf(outputFile, "\t}\n\n");
        printer::fprintf(outputFile, "\tinline double pow(const double x, const double n) {\n");
        printer::fprintf(outputFile, "\t\treturn std::pow(x, n);\n");
        printer::fprintf(outputFile, "\t}\n");
        printer::fprintf(outputFile, "}\n");
        printer::fprintf(outputFile, "#endif // JITEXPR_HELPER_FCT_H\n");
        fclose(outputFile);
    }
}

void spider::expr::CompiledExpression::compileExpression() {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwrite-strings"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
    constexpr char *const argv[] = { "g++", "-shared", "-o", "./.cache/libjitexpr.so", "./.cache/libjitexpr.cpp",
                                     "-std=c++11", "-O2", "-fPIC", "-lm", nullptr };
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    pid_t pid;
    if (posix_spawnp(&pid, "g++", nullptr, nullptr, argv, environ) != 0) {
        throwSpiderException("failed to compile expression.");
    }
    int status;
    if (waitpid(pid, &status, 0) != pid) {
        throwSpiderException("failed to compile expression.");
    }
}

spider::expr::CompiledExpression::functor_t
spider::expr::CompiledExpression::importExpression(const std::string &func) {
    if (!hndl_) {
        hndl_ = dlopen("./.cache/libjitexpr.so", RTLD_LAZY);
    }
    if (hndl_) {
        localHndlCpy_ = hndl_;
        auto *ptr = dlsym(hndl_, func.c_str());
        if (ptr) {
            return reinterpret_cast<functor_t>(ptr);
        }
    }
    throwSpiderException("failed to import compiled expression.");
}

spider::expr::CompiledExpression::~CompiledExpression() {
    if (hndl_) {
        dlclose(hndl_);
        hndl_ = nullptr;
    }
}

#endif
