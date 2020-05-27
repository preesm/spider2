/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
 *
 * Spider is a dataflow based runtime used to execute dynamic PiSDF
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

#ifdef __linux__

/* === Include(s) === */

#include <graphs-tools/expression-parser/helper/CompiledExpression.h>
#include <graphs/pisdf/Param.h>
#include <dlfcn.h>

/* === Function(s) definition === */

spider::expr::CompiledExpression::CompiledExpression(const spider::vector<RPNElement> &postfixStack,
                                                     const param_table_t &params) {
    /* == Tries to create the folder if it does not already exists == */
    if (system("mkdir -p ./jit-expr")) {
        throwSpiderException("failed to create directory for jit compiled expressions.");
    }
    /* == Convert string to C++ syntax == */
    const auto stack = convertToCpp(postfixStack);
    /* == Compute hash for equality == */
    hash_ = std::hash<std::string>{ }(rpn::postfixString(stack));
    /* == Perform partial evaluation of the expression (if possible) and compile the expression stack == */
    compile(stack, params);
}

double spider::expr::CompiledExpression::evaluate(const param_table_t &params) const {
    updateSymbolTable(params);
    return expr_(valueTable_.data());
}

/* === Private method(s) === */

spider::vector<RPNElement>
spider::expr::CompiledExpression::convertToCpp(const spider::vector<RPNElement> &postfixStack) const {
    auto res = factory::vector<RPNElement>(postfixStack, StackID::EXPRESSION);
    for (auto &e : res) {
        if (e.token_ == "^") {
            e.token_ = "std::pow";
            e.subtype_ = RPNElementSubType::FUNCTION;
        } else if (e.token_ == "and") {
            e.token_ = "and__";
        } else if (e.token_ == "or") {
            e.token_ = "or__";
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

void spider::expr::CompiledExpression::registerSymbol(param_t const param) {
    for (const auto &s : symbolTable_) {
        if (s == param->name()) {
            return;
        }
    }
    symbolTable_.emplace_back(param->name());
    valueTable_.emplace_back(0.);
}

void spider::expr::CompiledExpression::updateSymbolTable(const param_table_t &params) const {
    auto it = valueTable_.begin();
    for (const auto &sym : symbolTable_) {
        for (const auto &p : params) {
            if (sym == p->name()) {
                *(it++) = static_cast<double>(p->value(params));
                break;
            }
        }
    }
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
    const auto file = writeFunctionFile(func, rpn::infixString(postfixStack), symbolTable_);
    if (file == "__exists__") {
        /* == Import function == */
        expr_ = importExpression(std::string("./jit-expr/lib") + func + ".so", func);
    } else {
        /* == Invoke g++ to compile expression == */
        const auto lib = compileExpression(func);
        /* == Import function == */
        expr_ = importExpression(lib, func);
    }
}

std::string spider::expr::CompiledExpression::writeFunctionFile(const std::string &func,
                                                                const std::string &expression,
                                                                const spider::vector<std::string> &args) const {
    const auto fileName = std::string("./jit-expr/") + func + ".cpp";
    /* == Check if file already exists == */
    if (FILE *file = fopen(fileName.c_str(), "r")) {
        fclose(file);
        return "__exists__";
    }
    FILE *outputFile = fopen(fileName.c_str(), "w+");
    if (outputFile) {
        fprintf(outputFile, "#include <cmath>\n");
        fprintf(outputFile, "#include <functional>\n\n");
        fprintf(outputFile, "#define if(x, y, z) ((x) ? (y) : (z))\n");
        fprintf(outputFile, "extern \"C\" {\n");
        fprintf(outputFile, "\tconst double and__(const double x, const double y) {\n");
        fprintf(outputFile,
                "\t\treturn std::not_equal_to<double>{ }(0., x) && std::not_equal_to<double>{ }(0., y) ? 1. : 0.;\n");
        fprintf(outputFile, "\t}\n\n");
        fprintf(outputFile, "\tconst double or__(const double x, const double y) {\n");
        fprintf(outputFile,
                "\t\treturn std::not_equal_to<double>{ }(0., x) || std::not_equal_to<double>{ }(0., y) ? 1. : 0.;\n");
        fprintf(outputFile, "\t}\n\n");
        fprintf(outputFile, "\tdouble %s(const double *args) {\n", func.c_str());
        for (size_t i = 0; i < args.size(); ++i) {
            fprintf(outputFile, "\t\tconst auto %s = args[%zuu];\n", args[i].c_str(), i);
        }
        fprintf(outputFile, "\t\treturn %s;\n", expression.c_str());
        fprintf(outputFile, "\t}\n");
        fprintf(outputFile, "}\n");
        fprintf(outputFile, "#undef if");
        fclose(outputFile);
        return fileName;
    }
    return "";
}

std::string spider::expr::CompiledExpression::compileExpression(const std::string &func) const {
    const auto lib = std::string("./jit-expr/lib") + func + ".so";
    const auto cpp = std::string("./jit-expr/") + func + ".cpp";
    const auto cmd = std::string("g++ -shared -o ") + lib + " " + cpp + " -O2";
    if (system(cmd.c_str())) {
        throwSpiderException("failed to compile expression.");
    }
    return lib;
}

std::function<double(const double *)>
spider::expr::CompiledExpression::importExpression(const std::string &lib, const std::string &func) {
    hndl_ = std::shared_ptr<void>(dlopen(lib.c_str(), RTLD_LAZY), dlclose);
    if (hndl_) {
        auto *ptr = dlsym(hndl_.get(), func.c_str());
        if (ptr) {
            return [ptr](const double *args) { return reinterpret_cast<functor_t>(ptr)(args); };
        }
    }
    throwSpiderException("failed to import compiled expression.");
}

#endif
