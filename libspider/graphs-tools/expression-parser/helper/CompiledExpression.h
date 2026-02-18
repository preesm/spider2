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
#ifndef SPIDER2_COMPILEDEXPRESSION_H
#define SPIDER2_COMPILEDEXPRESSION_H

#if defined(__linux__) && defined(_SPIDER_JIT_EXPRESSION)

/* === Include(s) === */

#include <containers/vector.h>
#include <graphs-tools/expression-parser/RPNConverter.h>

/* === Function(s) prototype === */

namespace spider {
    namespace expr {

        namespace details {
            static inline void cleanFolder() {
                if (system("rm -rf ./.cache")) {
                    throwSpiderException("failed to clean jit expression folder.");
                }
            }
        }

        class CompiledExpression {
        public:
            using param_t = pisdf::Param *;
            using param_table_t = spider::vector<std::shared_ptr<pisdf::Param>>;
            using functor_t = double (*)(const double *);

            CompiledExpression(const spider::vector<RPNElement> &postfixStack, const param_table_t &params);

            CompiledExpression(const CompiledExpression &) = default;

            CompiledExpression(CompiledExpression &&) = default;

            ~CompiledExpression();

            /* === Operator(s) === */

            CompiledExpression &operator=(const CompiledExpression &) = default;

            CompiledExpression &operator=(CompiledExpression &&) = default;

            inline bool operator==(const CompiledExpression &rhs) const { return hash_ == rhs.hash_; }

            inline bool operator!=(const CompiledExpression &rhs) const { return !(*this == rhs); }

            /* === Method(s) === */

            double evaluate(const param_table_t &params = { });

        private:

            /* === Private members === */

            static void* hndl_;
            void *localHndlCpy_ = nullptr;
            spider::vector<double> valueTable_;
            spider::vector<std::pair<size_t, std::string>> symbolTable_;
            functor_t expr_{ };
            size_t hash_{ SIZE_MAX };

            /* === Private method(s) === */

            static param_t findParameter(const param_table_t &params, const std::string &name);

            void registerSymbol(param_t param);

            void updateSymbolTable(const param_table_t &params);

            static spider::vector<RPNElement> convertToCpp(const spider::vector<RPNElement> &postfixStack) ;

            /**
             * @brief Compile the expression if needed.
             * @param postfixStack Post fix of the expression.
             * @param params       Parameters used for evaluation.
             */
            void compile(const spider::vector<RPNElement> &postfixStack, const param_table_t &params);

            /**
             * @brief Write the cpp function file.
             * @param func         Name of the function to generate.
             * @param expression   Infix string of the expression to compile.
             * @param args         Arguments of the expression.
             */
            static void writeFunctionFile(const std::string &func,
                                   const std::string &expression,
                                   const spider::vector<std::pair<size_t, std::string>> &args) ;

            /**
             * @brief Write the .h file with custom functions (if it does not exists).
             */
            static void writeHelperFile() ;

            /**
             * @brief Perform just in time compilation of the expression.
             * @throw @refitem spider::Exception if failed to compile.
             */
            static void compileExpression() ;

            /**
             * @brief Import the function from the compiled library.
             * @param func   Function name.
             * @return imported function.
             * @throw @refitem spider::Exception if failed to import.
             */
            functor_t importExpression(const std::string &func);

        };
    }
}

#endif
#endif //SPIDER2_COMPILEDEXPRESSION_H
