/*
 * This project is dual licensed under the BSD 3-Clause License and under the Apache License version 2.0
 *
 * --------------------------------------------------------------------------------
 * BSD 3-Clause License
 *
 * Copyright (c) 2019, Evan Teran
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  Redistributions of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 *    Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --------------------------------------------------------------------------------
 * Copyright 2019 Evan Teran
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * This code is originated from https://github.com/eteran/cxx11_printf.
 * this particular header is extracted from the original Printf.h header.
 */
#ifndef SPIDER2_ITOA_H
#define SPIDER2_ITOA_H

/* === Includes === */

#include <cstdint>
#include <string>
#include <cassert>
#include <algorithm>

/* === Define(s) === */

#define CXX11_PRINTF_EXTENSIONS

/* === Methods prototype === */

namespace spider {
    namespace cxx11 {
        namespace detail {
            enum class Modifiers {
                MOD_NONE,
                MOD_CHAR,
                MOD_SHORT,
                MOD_LONG,
                MOD_LONG_LONG,
                MOD_LONG_DOUBLE,
                MOD_INTMAX_T,
                MOD_SIZE_T,
                MOD_PTRDIFF_T
            };

            struct Flags {
                uint8_t justify  : 1;
                uint8_t sign     : 1;
                uint8_t space    : 1;
                uint8_t prefix   : 1;
                uint8_t padding  : 1;
                uint8_t reserved : 3;
            };

            // NOTE(eteran): by placing this in a class, it allows us to do things like specialization a lot easier
            template<uint32_t Divisor>
            struct itoa_helper;

            template<>
            struct itoa_helper<10> {
                static constexpr uint32_t Divisor = 10;

            public:
                //------------------------------------------------------------------------------
                // Name: format
                // Desc: returns the value of d as a C-string, formatted based on Divisor,
                //       and flags. places the length of the resultant string in *rlen
                //------------------------------------------------------------------------------
                template<class T, size_t N>
                static const char *
                format(char (&buf)[N], T d, int32_t width, Flags flags, const char *alphabet, size_t *rlen) {
                    auto ud = static_cast<typename std::make_unsigned<T>::type>(d);

                    char *p = buf + N;
                    *--p = '\0';

                    // reserve space for leading chars as needed
                    // and if necessary negate the value in ud
#if defined(_MSC_VER)
                    // For some reason, visual is not able to see that T is necessary signed here so we silence the warning
                    // Also, visual does not seem to like constant expression evaluation..
#pragma warning(push)
#pragma warning(disable: 4146)
#pragma warning(disable: 4127)
#endif
                    if (std::is_signed<T>::value && d < 0) {
                        d = static_cast<T>(-d);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
                        ud = static_cast<typename std::make_unsigned<T>::type>(d);
                        width -= 1;
                    } else if (flags.space) {
                        width -= 1;
                    } else if (flags.sign) {
                        width -= 1;
                    }

                    // Divide UD by Divisor until UD == 0.
                    int32_t digits = 0;
                    if (!ud) {
                        *--p = '0';
                    } else {
                        while (ud) {
                            const auto remainder = static_cast<int32_t>(ud % Divisor);
                            *--p = alphabet[remainder];
                            ++digits;
                            ud = static_cast<typename std::make_unsigned<T>::type>(ud / Divisor);
                        }
                    }

                    // add in any necessary padding
                    if (flags.padding) {
                        while (width-- > digits) {
                            *--p = '0';
                        }
                    }

                    // add the prefix as needed
                    if (d < 0) {
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

// Specialization for base 16 so we can make some assumptions
            template<>
            struct itoa_helper<16> {
                static constexpr uint32_t Shift = 4;
                static constexpr uint32_t Mask = 0x0f;

            public:
                //------------------------------------------------------------------------------
                // Name: format
                // Desc: returns the value of d as a C-string, formatted based on Divisor,
                //       and flags. places the length of the resultant string in *rlen
                //------------------------------------------------------------------------------
                template<class T, size_t N>
                static const char *
                format(char (&buf)[N], T d, int32_t width, Flags flags, const char *alphabet, size_t *rlen) {
                    auto ud = static_cast<typename std::make_unsigned<T>::type>(d);

                    char *p = buf + N;
                    *--p = '\0';

                    // add the prefix as needed
                    if (flags.prefix) {
                        width -= 2;
                    }

                    // Divide UD by Divisor until UD == 0.
                    int32_t digits = 0;
                    while (ud) {
                        const int32_t remainder = (ud & Mask);
                        *--p = alphabet[remainder];
                        ++digits;
                        ud = static_cast<typename std::make_unsigned<T>::type>(ud >> Shift);
                    }

                    // add in any necessary padding
                    if (flags.padding) {
                        while (width-- > digits) {
                            *--p = '0';
                        }
                    }

                    // add the prefix as needed
                    if (flags.prefix) {
                        *--p = alphabet[16];
                        *--p = '0';
                    }

                    *rlen = static_cast<size_t>((buf + N) - p - 1);
                    return p;
                }
            };

// Specialization for base 8 so we can make some assumptions
            template<>
            struct itoa_helper<8> {
                static constexpr uint32_t Shift = 3;
                static constexpr uint32_t Mask = 0x07;

            public:
                //------------------------------------------------------------------------------
                // Name: format
                // Desc: returns the value of d as a C-string, formatted based on Divisor,
                //       and flags. places the length of the resultant string in *rlen
                //------------------------------------------------------------------------------
                template<class T, size_t N>
                static const char *
                format(char (&buf)[N], T d, int32_t width, Flags flags, const char *alphabet, size_t *rlen) {
                    auto ud = static_cast<typename std::make_unsigned<T>::type>(d);

                    char *p = buf + N;
                    *--p = '\0';

                    // add the prefix as needed
                    if (flags.prefix) {
                        width -= 1;
                    }

                    // Divide UD by Divisor until UD == 0.
                    int32_t digits = 0;
                    while (ud) {
                        const int32_t remainder = (ud & Mask);
                        *--p = alphabet[remainder];
                        ++digits;
                        auto newi = ud >> Shift;
                        ud = static_cast<typename std::make_unsigned<T>::type>(newi);
                    }

                    // add in any necessary padding
                    if (flags.padding) {
                        while (width-- > digits) {
                            *--p = '0';
                        }
                    }

                    // add the prefix as needed
                    if (flags.prefix) {
                        *--p = '0';
                    }

                    *rlen = static_cast<size_t>((buf + N) - p - 1);
                    return p;
                }
            };

// Specialization for base 2 so we can make some assumptions
            template<>
            struct itoa_helper<2> {
                static constexpr uint32_t Shift = 1;
                static constexpr uint32_t Mask = 0x01;

            public:
                //------------------------------------------------------------------------------
                // Name: format
                // Desc: returns the value of d as a C-string, formatted based on Divisor,
                //       and flags. places the length of the resultant string in *rlen
                //------------------------------------------------------------------------------
                template<class T, size_t N>
                static const char *
                format(char (&buf)[N], T d, int32_t width, Flags flags, const char *alphabet, size_t *rlen) {
                    auto ud = static_cast<typename std::make_unsigned<T>::type>(d);

                    char *p = buf + N;
                    *--p = '\0';

                    // add the prefix as needed
                    if (flags.prefix) {
                        width -= 2;
                    }

                    // Divide UD by Divisor until UD == 0.
                    int32_t digits = 0;
                    while (ud) {
                        const int32_t remainder = (ud & Mask);
                        *--p = alphabet[remainder];
                        ++digits;
                        ud = static_cast<typename std::make_unsigned<T>::type>(ud >> Shift);
                    }

                    // add in any necessary padding
                    if (flags.padding) {
                        while (width-- > digits) {
                            *--p = '0';
                        }
                    }

                    // add the prefix as needed
                    if (flags.prefix) {
                        *--p = 'b';
                        *--p = '0';
                    }

                    *rlen = static_cast<size_t>((buf + N) - p - 1);
                    return p;
                }
            };

//------------------------------------------------------------------------------
// Name: itoa
// Desc: as a minor optimization, let's determine a few things up front and pass
//       them as template parameters enabling some more aggressive optimizations
//       when the division can use more efficient operations
//------------------------------------------------------------------------------
            template<class T, size_t N>
            const char *
            itoa(char (&buf)[N], char base, int32_t precision, T d, int32_t width, Flags flags, size_t *rlen) {

                if (d == 0 && precision == 0) {
                    *buf = '\0';
                    *rlen = 0;
                    return buf;
                }

                // NOTE(eteran): we include the x/X, here as an easy way to put the
                //               upper/lower case prefix for hex numbers
                static const char alphabet_l[] = "0123456789abcdefx";
                static const char alphabet_u[] = "0123456789ABCDEFX";

                switch (base) {
                    case 'i':
                    case 'd':
                    case 'u':
                        return itoa_helper<10>::format(buf, d, width, flags, alphabet_l, rlen);
#ifdef CXX11_PRINTF_EXTENSIONS
                    case 'b':
                        return itoa_helper<2>::format(buf, d, width, flags, alphabet_l, rlen);
#endif
                    case 'X':
                        return itoa_helper<16>::format(buf, d, width, flags, alphabet_u, rlen);
                    case 'x':
                        return itoa_helper<16>::format(buf, d, width, flags, alphabet_l, rlen);
                    case 'o':
                        return itoa_helper<8>::format(buf, d, width, flags, alphabet_l, rlen);
                    default:
                        return itoa_helper<10>::format(buf, d, width, flags, alphabet_l, rlen);
                }
            }
        }
    }
}

#endif //SPIDER2_ITOA_H
