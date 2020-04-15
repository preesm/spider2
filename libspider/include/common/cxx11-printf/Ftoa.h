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
#ifndef SPIDER2_FTOA_H
#define SPIDER2_FTOA_H

/* === Includes === */

#include <cmath>
#include <limits>
#include <common/cxx11-printf/Itoa.h>

/* === Methods prototype === */

namespace spider {
    namespace cxx11 {
        namespace detail {
            template<char format>
            struct ftoa_helper;

            template<>
            struct ftoa_helper<'f'> {
            public:
                template<class T, size_t N>
                static const char *
                format(char (&buf)[N], T f, int32_t width, int32_t precision, Flags flags, const char *alphabet,
                       size_t *rlen) {
                    auto negValue = f < 0;
                    if (negValue) {
                        f = -f;
                        width -= 1;
                    } else if (flags.space) {
                        width -= 1;
                    } else if (flags.sign) {
                        width -= 1;
                    }
                    int32_t digits = 0;
                    char *p = buf;
                    if (f == 0.) {
                        digits = precision ? precision + 2 : 1;
                        p = buf + N;
                        *--p = '\0';
                        if (precision) {
                            for (int32_t i = 0; i < precision; ++i) {
                                *--p = '0';
                            }
                            *--p = '.';
                        }
                        *--p = '0';
                    } else {
                        auto exp = static_cast<int32_t>(std::log10(f));
                        if (exp < 0) {
                            exp = 0;
                        }
                        int32_t currentPrecision = 0;
                        while (exp >= 0 || currentPrecision < precision) {
                            auto weight = std::pow(10, exp);

                            /* == Add current digit == */
                            if (weight > 0 && !std::isinf(weight)) {
                                const auto div = static_cast<int32_t>(std::floor(f / weight));
                                auto digit = alphabet[div];
                                *(p++) = digit;
                                f -= (weight * div);
                                digits += 1;
                            }

                            /* == Add the decimal point == */
                            if (!exp) {
                                *(p++) = '.';
                                digits += 1;
                            }

                            if (weight < 1.) {
                                currentPrecision += 1;
                            }

                            /* == Decrement exponent == */
                            exp -= 1;
                        }

                        /* == Reverse the notation to match itoa == */
                        p = buf + N;
                        *--p = '\0';
                        for (int32_t i = digits - 1; i >= 0; --i) {
                            *--p = buf[i];
                        }
                    }

                    /* == Add in any necessary padding == */
                    if (flags.padding) {
                        while (width-- > digits) {
                            *--p = '0';
                        }
                    }

                    /* == Add the prefix as needed == */
                    if (negValue) {
                        *--p = '-';
                    } else if (flags.space) {
                        *--p = ' ';
                    } else if (flags.sign) {
                        *--p = '+';
                    }

                    *rlen = static_cast<size_t>((buf + N) - p - 1);
                    return p;
                }
            };

            template<>
            struct ftoa_helper<'e'> {
            public:
                template<class T, size_t N>
                static const char *
                format(char (&buf)[N], T f, int32_t width, int32_t precision, Flags flags, const char *alphabet,
                       size_t *rlen) {
                    auto exp = static_cast<int32_t>(std::log10(f));
                    T val = f / std::pow(10., exp);
                    ftoa_helper<'f'>::format(buf, val, width, precision, flags, alphabet, rlen);
                    /* == We assume that we have the space for writing the exponent == */
                    /* == Double have precision of ~15 digits, so with 131 char, this should not pose any problem == */
                    char *r_p = buf;
                    *(r_p++) = 'e';
                    if (exp < 0) {
                        *(r_p++) = '-';
                    } else {
                        *(r_p++) = '+';
                    }
                    int32_t ud = std::abs(exp);

                    /* == Write the exponent == */
                    int32_t digits = 2;
                    if (ud > 9) {
                        for (; ud; ud /= 10) {
                            const int32_t remainder = (ud % 10);
                            *(r_p++) = alphabet[remainder];
                            digits += 1;
                        }
                    } else {
                        digits = 4;
                        *(r_p++) = '0';
                        *r_p = alphabet[(ud % 10)];
                    }

                    /* == Now lets move everything == */
                    char *p = buf + N - 1 - *rlen;
                    std::memmove(p - digits, p, (*rlen) * sizeof(char));
                    p = buf + N - 1;
                    for (int32_t i = digits - 1; i >= 0; --i) {
                        *--p = buf[i];
                    }

                    *rlen = *rlen + static_cast<unsigned long>(digits);
                    return buf + N - 1 - *rlen;
                }
            };

            template<>
            struct ftoa_helper<'g'> {
            public:
                template<class T, size_t N>
                static const char *
                format(char (&buf)[N], T f, int32_t width, int32_t precision, Flags flags, const char *alphabet,
                       size_t *rlen) {
                    if (precision == 0) {
                        precision = 1;
                    }
                    if (f == 0.) {
                        char *p = buf + N;
                        *--p = '\0';
                        *--p = '0';
                        *rlen = 1;
                        return p;
                    }

                    auto exp = static_cast<int32_t>(std::log10(f));

                    /* == Exponential notation is NOT used if given precision P and exponent X:  -4 <= X < P == */
                    bool useExpNotation = !((exp < precision) && (exp >= -4));

                    /* == Fall back to ftoa_helper<'f'>::format == */
                    if (!useExpNotation) {
                        if (flags.prefix) {
                            return ftoa_helper<'f'>::format(buf, f, width, precision - (exp + 1), flags, alphabet,
                                                            rlen);
                        } else {
                            /* == Remove trailing '0' == */
                            ftoa_helper<'f'>::format(buf, f, width, precision - (exp + 1), flags, alphabet, rlen);
                            char *p = buf + N - 1 - *rlen;
                            size_t zeros = 0;
                            char *rp = buf + N - 1;
                            while (*(--rp) == '0') {
                                zeros += 1;
                            }
                            auto *dest = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(p) + *(rlen) - zeros);
                            std::memmove(dest, p, (*(rlen) - zeros) * sizeof(char));
                            (*rlen) = (*rlen) - zeros;
                            return buf + N - 1 - *(rlen);
                        }
                    }

                    /* == Fall back to ftoa_helper<'e'>::format == */
                    return ftoa_helper<'e'>::format(buf, f, width, precision - 1, flags, alphabet, rlen);
                }
            };


            template<class T, size_t N>
            const char *
            ftoa(char (&buf)[N], char base, int32_t precision, T f, int32_t width, Flags flags, size_t *rlen) {
                if (std::isnan(f)) {
                    if (base == 'f' || base == 'g' || base == 'e') {
                        buf[0] = 'n';
                        buf[1] = 'a';
                        buf[2] = 'n';
                    } else {
                        buf[0] = 'N';
                        buf[1] = 'a';
                        buf[2] = 'N';
                    }
                    *buf = '\0';
                    *rlen = 3;
                    return buf;
                } else if (std::isinf(f)) {
                    if (base == 'f' || base == 'g' || base == 'e') {
                        buf[0] = 'i';
                        buf[1] = 'n';
                        buf[2] = 'f';
                    } else {
                        buf[0] = 'I';
                        buf[1] = 'N';
                        buf[2] = 'F';
                    }
                    *buf = '\0';
                    *rlen = 3;
                    return buf;
                }

                if (f == 0 && precision == 0) {
                    *buf = '\0';
                    *rlen = 0;
                    return buf;
                }

                static const char alphabet[] = "0123456789";
                switch (base) {
                    case 'f':
                    case 'F':
                        return ftoa_helper<'f'>::format(buf, f, width, precision, flags, alphabet, rlen);
                    case 'g':
                    case 'G':
                        return ftoa_helper<'g'>::format(buf, f, width, precision, flags, alphabet, rlen);
                    case 'e':
                    case 'E':
                        return ftoa_helper<'e'>::format(buf, f, width, precision, flags, alphabet, rlen);
                    default:
                        return ftoa_helper<'f'>::format(buf, f, width, precision, flags, alphabet, rlen);
                }
            }

        }
    }
}

#endif //SPIDER2_FTOA_H
