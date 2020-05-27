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
#ifndef SPIDER2_EXPRESSIONTOKEN_H
#define SPIDER2_EXPRESSIONTOKEN_H

/* === Include(s) === */

#include <functional>
#include <containers/vector.h>

/* === Function(s) prototype === */

namespace spider {
    namespace expr {

        struct Token {
            using symbol_t = std::pair<std::string, double>;
            using symbol_table_t = spider::vector<symbol_t>;
            using functor_t = std::function<double(const symbol_table_t &)>;

            enum TokenType {
                NONE,
                CONSTANT,
                VARIABLE,
                FUNCTION
            };

            Token() = default;

            Token(double v) : f_{ [v](const symbol_table_t &) { return v; }},
                              value_{ v },
                              type_{ Token::CONSTANT } { }

            Token(size_t i) : f_{ [i](const symbol_table_t &t) { return t[i].second; }},
                              index_{ i },
                              type_{ Token::VARIABLE } { }

            Token(functor_t f) : f_{ std::move(f) }, type_{ Token::FUNCTION } { }

            inline double operator()(const symbol_table_t &t) const { return f_(t); }

            inline TokenType type() const { return type_; }

            functor_t f_{ };
            double value_{ };
            size_t index_{ };
            TokenType type_;
        };

        namespace details {

            inline bool isConstConst(const Token &b0, const Token &b1) {
                return Token::CONSTANT == b0.type() && Token::CONSTANT == b1.type();
            }

            inline bool isVarVar(const Token &b0, const Token &b1) {
                return Token::VARIABLE == b0.type() && Token::VARIABLE == b1.type();
            }

            inline bool isConstVar(const Token &b0, const Token &b1) {
                return Token::CONSTANT == b0.type() && Token::VARIABLE == b1.type();
            }

            inline bool isVarConst(const Token &b0, const Token &b1) {
                return Token::VARIABLE == b0.type() && Token::CONSTANT == b1.type();
            }

            inline bool isConstFunc(const Token &b0, const Token &b1) {
                return Token::CONSTANT == b0.type() && Token::FUNCTION == b1.type();
            }

            inline bool isFuncConst(const Token &b0, const Token &b1) {
                return Token::FUNCTION == b0.type() && Token::CONSTANT == b1.type();
            }

            inline bool isVarFunc(const Token &b0, const Token &b1) {
                return Token::VARIABLE == b0.type() && Token::FUNCTION == b1.type();
            }

            inline bool isFuncVar(const Token &b0, const Token &b1) {
                return Token::FUNCTION == b0.type() && Token::VARIABLE == b1.type();
            }
        }
    }
}

#endif //SPIDER2_EXPRESSIONTOKEN_H
