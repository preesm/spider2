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
#ifndef SPIDER2_EXPRESSIONNUMERIC_H
#define SPIDER2_EXPRESSIONNUMERIC_H

/* === Include(s) === */

#include <graphs-tools/expression-parser/RPNConverter.h>

/* === Function(s) prototype === */

namespace spider {
    namespace numeric {
        namespace details {

            inline bool isTrue(const double v) {
                return std::not_equal_to<double>{ }(0., v);
            }

#ifdef _WIN32

            /* === Unary functions === */

            struct fact {
                static inline double apply(const double v) { return math::factorial(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::FACT; }
            };

            struct cos {
                static inline double apply(const double v) { return std::cos(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::COS; }
            };

            struct sin {
                static inline double apply(const double v) { return std::sin(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::SIN; }
            };

            struct tan {
                static inline double apply(const double v) { return std::tan(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::TAN; }
            };

            struct cosh {
                static inline double apply(const double v) { return std::cosh(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::COSH; }
            };

            struct sinh {
                static inline double apply(const double v) { return std::sinh(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::SINH; }
            };

            struct tanh {
                static inline double apply(const double v) { return std::tanh(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::TANH; }
            };

            struct exp {
                static inline double apply(const double v) { return std::exp(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::EXP; }
            };

            struct log {
                static inline double apply(const double v) { return std::log(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::LOG; }
            };

            struct log2 {
                static inline double apply(const double v) { return std::log2(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::LOG2; }
            };

            struct log10 {
                static inline double apply(const double v) { return std::log10(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::LOG10; }
            };

            struct ceil {
                static inline double apply(const double v) { return std::ceil(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::CEIL; }
            };

            struct floor {
                static inline double apply(const double v) { return std::floor(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::FLOOR; }
            };

            struct abs {
                static inline double apply(const double v) { return v < 0. ? (-v) : v; }

                static inline RPNOperatorType type() { return RPNOperatorType::ABS; }
            };

            struct sqrt {
                static inline double apply(const double v) { return std::sqrt(v); }

                static inline RPNOperatorType type() { return RPNOperatorType::SQRT; }
            };

            /* === Binary functions === */

            struct add {
                static inline double apply(const double v0, const double v1) { return v0 + v1; }

                static inline RPNOperatorType type() { return RPNOperatorType::ADD; }
            };

            struct mul {
                static inline double apply(const double v0, const double v1) { return v0 * v1; }

                static inline RPNOperatorType type() { return RPNOperatorType::MUL; }
            };

            struct sub {
                static inline double apply(const double v0, const double v1) { return v0 - v1; }

                static inline RPNOperatorType type() { return RPNOperatorType::SUB; }
            };

            struct div {
                static inline double apply(const double v0, const double v1) { return v0 / v1; }

                static inline RPNOperatorType type() { return RPNOperatorType::DIV; }
            };

            struct pow {
                static inline double apply(const double v0, const double v1) {
                    if (v1 < 100. && (std::trunc(v1) == v1)) {
                        auto res{ v0 };
                        auto n{ v1 };
                        while (n > 1.) {
                            res *= v0;
                            n -= 1;
                        }
                        return res;
                    }
                    return std::pow(v0, v1);
                }

                static inline RPNOperatorType type() { return RPNOperatorType::POW; }
            };

            struct mod {
                static inline double apply(const double v0, const double v1) { return std::fmod(v0, v1); }

                static inline RPNOperatorType type() { return RPNOperatorType::MOD; }
            };

            struct max {
                static inline double apply(const double v0, const double v1) { return std::max(v0, v1); }

                static inline RPNOperatorType type() { return RPNOperatorType::MAX; }
            };

            struct min {
                static inline double apply(const double v0, const double v1) { return std::min(v0, v1); }

                static inline RPNOperatorType type() { return RPNOperatorType::MIN; }
            };

            struct land {
                static inline double apply(const double v0, const double v1) {
                    return details::isTrue(v0) && details::isTrue(v1) ? 1. : 0.;
                }

                static inline RPNOperatorType type() { return RPNOperatorType::LOG_AND; }
            };

            struct lor {
                static inline double apply(const double v0, const double v1) {
                    return details::isTrue(v0) || details::isTrue(v1) ? 1. : 0.;
                }

                static inline RPNOperatorType type() { return RPNOperatorType::LOG_OR; }
            };

            struct gt {
                static inline double apply(const double v0, const double v1) { return v0 > v1 ? 1. : 0.; }

                static inline RPNOperatorType type() { return RPNOperatorType::LOG_OR; }
            };

            struct gte {
                static inline double apply(const double v0, const double v1) { return v0 >= v1 ? 1. : 0.; }

                static inline RPNOperatorType type() { return RPNOperatorType::LOG_OR; }
            };

            struct lt {
                static inline double apply(const double v0, const double v1) { return v0 < v1 ? 1. : 0.; }

                static inline RPNOperatorType type() { return RPNOperatorType::LOG_OR; }
            };

            struct lte {
                static inline double apply(const double v0, const double v1) { return v0 <= v1 ? 1. : 0.; }

                static inline RPNOperatorType type() { return RPNOperatorType::LOG_OR; }
            };

#endif
        }

        /**
         * @brief Apply an operator on given argument.
         * @param type    Operator type.
         * @param arg0    Argument.
         * @return result of the operator.
         * @throw spider::Exception if operator does not exist.
         */
        inline double apply(RPNOperatorType type, const double arg0) {
            switch (type) {
                case RPNOperatorType::FACT:
                    return math::factorial(arg0);
                case RPNOperatorType::COS:
                    return std::cos(arg0);
                case RPNOperatorType::SIN:
                    return std::sin(arg0);
                case RPNOperatorType::TAN:
                    return std::tan(arg0);
                case RPNOperatorType::COSH:
                    return std::cosh(arg0);
                case RPNOperatorType::SINH:
                    return std::sinh(arg0);
                case RPNOperatorType::TANH:
                    return std::tanh(arg0);
                case RPNOperatorType::EXP:
                    return std::exp(arg0);
                case RPNOperatorType::LOG:
                    return std::log(arg0);
                case RPNOperatorType::LOG2:
                    return std::log2(arg0);
                case RPNOperatorType::LOG10:
                    return std::log10(arg0);
                case RPNOperatorType::CEIL:
                    return std::ceil(arg0);
                case RPNOperatorType::FLOOR:
                    return std::floor(arg0);
                case RPNOperatorType::ABS:
                    return math::abs(arg0);
                case RPNOperatorType::SQRT:
                    return std::sqrt(arg0);
                default:
                    if (log::enabled<log::EXPR>()) {
                        log::warning<log::EXPR>("Invalid unary operation.");
                    }
                    return std::numeric_limits<double>::quiet_NaN();
            }
        }

        /**
         * @brief Apply an operator on given arguments.
         * @param type    Operator type.
         * @param arg0    Argument 0 of operator.
         * @param arg1    Argument 1 of operator.
         * @return result of the operator.
         * @throw spider::Exception if operator does not exist.
         */
        inline double apply(RPNOperatorType operation, const double arg0, const double arg1) {
            switch (operation) {
                case RPNOperatorType::ADD:
                    return arg0 + arg1;
                case RPNOperatorType::SUB:
                    return arg0 - arg1;
                case RPNOperatorType::MUL:
                    return arg0 * arg1;
                case RPNOperatorType::DIV:
                    return arg0 / arg1;
                case RPNOperatorType::MOD:
                    return std::fmod(arg0, arg1);
                case RPNOperatorType::POW:
                    return std::pow(arg0, arg1);
                case RPNOperatorType::MAX:
                    return std::max(arg0, arg1);
                case RPNOperatorType::MIN:
                    return std::min(arg0, arg1);
                case RPNOperatorType::LOG_AND:
                    return details::isTrue(arg0) && details::isTrue(arg1) ? 1. : 0.;
                case RPNOperatorType::LOG_OR:
                    return details::isTrue(arg0) || details::isTrue(arg1) ? 1. : 0.;
                case RPNOperatorType::GREATER:
                    return arg0 > arg1 ? 1. : 0.;
                case RPNOperatorType::GEQ:
                    return arg0 >= arg1 ? 1. : 0.;
                case RPNOperatorType::LESS:
                    return arg0 < arg1 ? 1. : 0.;
                case RPNOperatorType::LEQ:
                    return arg0 <= arg1 ? 1. : 0.;
                default:
                    if (log::enabled<log::EXPR>()) {
                        log::warning<log::EXPR>("Invalid binary operation.");
                    }
                    return std::numeric_limits<double>::quiet_NaN();
            }
        }

        /**
         * @brief Apply an operator on given arguments.
         * @param type    Operator type.
         * @param arg0    Argument 0 of operator.
         * @param arg1    Argument 1 of operator.
         * @param arg2    Argument 2 of operator.
         * @return result of the operator.
         * @throw spider::Exception if operator does not exist.
         */
        inline double apply(RPNOperatorType type, const double arg0, const double arg1, const double arg2) {
            switch (type) {
                case RPNOperatorType::IF:
                    return arg0 >= 1. ? arg1 : arg2;
                default:
                    if (log::enabled<log::EXPR>()) {
                        log::warning<log::EXPR>("Invalid ternary operation.");
                    }
                    return std::numeric_limits<double>::quiet_NaN();
            }
        }
    }
}

#endif //SPIDER2_EXPRESSIONNUMERIC_H
