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
 * Slight modifications have been done in order to uniformize with the rest of the Spider code base and add float support.
 */
#ifndef PRINTF_20160922_H_
#define PRINTF_20160922_H_

#include <common/cxx11-printf/Formatters.h>
#include <common/cxx11-printf/Itoa.h>
#include <common/cxx11-printf/Ftoa.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>

#define CXX11_PRINTF_EXTENSIONS

namespace spider {
    namespace cxx11 {
        struct format_error : std::runtime_error {
            explicit format_error(const char *what_arg) : std::runtime_error(what_arg) {
            };
        };

        namespace detail {
            static_assert(sizeof(Flags) == sizeof(uint8_t), "");

            //------------------------------------------------------------------------------
            // Name: output_string
            // Desc: prints a string to the Context object, taking into account padding flags
            // Note: ch is the current format specifier
            //------------------------------------------------------------------------------
            template<class Context>
            void
            output_string(char ch, const char *s_ptr, int32_t precision, int32_t width, Flags flags, int32_t len,
                          Context &ctx) {

                if ((ch == 's' && precision >= 0 && precision < len)) {
                    len = precision;
                }

                // if not left justified padding goes first...
                if (!flags.justify) {
                    // spaces go before the prefix...
                    while (width-- > len) {
                        ctx.write(' ');
                    }
                }

                // output the string
                // NOTE(eteran): len is at most strlen, possible is less
                // so we can just loop len times
                width -= len;
                ctx.write(s_ptr, static_cast<size_t>(len));

                // if left justified padding goes last...
                if (flags.justify) {
                    while (width-- > 0) {
                        ctx.write(' ');
                    }
                }
            }

// NOTE(eteran): Here is some code to fetch arguments of specific types. We also need a few
//               default handlers, this code should never really be encountered, but
//               but we need it to keep the linker happy.

#ifdef CXX11_PRINTF_EXTENSIONS

            inline std::string formatted_object(std::string obj) {
                return obj;
            }

            template<class T>
            std::string to_string(T) {
                throw format_error("No to_string found for this object type");
            }

            template<class T>
            std::string formatted_object(T obj) {
                using std::to_string;
                using detail::to_string;
                return to_string(obj);
            }

#endif

            template<class T>
            constexpr const char *
            formatted_string(T s, typename std::enable_if<std::is_convertible<T, const char *>::value>::type * = 0) {
                return s;
            }

            template<class T>
            const char *
            formatted_string(T s, typename std::enable_if<!std::is_convertible<T, const char *>::value>::type * = 0) {
                (void) s;
                throw format_error("Non-String Argument For String Format");
            }

            template<class R, class T>
            constexpr R
            formatted_pointer(T p, typename std::enable_if<std::is_convertible<T, const void *>::value>::type * = 0) {
                return reinterpret_cast<R>(reinterpret_cast<uintptr_t>(p));
            }

            template<class R, class T>
            R
            formatted_pointer(T p, typename std::enable_if<!std::is_convertible<T, const void *>::value>::type * = 0) {
                (void) p;
                throw format_error("Non-Pointer Argument For Pointer Format");
            }

            template<class R, class T>
            constexpr R formatted_integer(T n, typename std::enable_if<std::is_integral<T>::value>::type * = 0) {
                return static_cast<R>(n);
            }

            template<class R, class T>
            R formatted_integer(T n, typename std::enable_if<!std::is_integral<T>::value>::type * = 0) {
                (void) n;
                throw format_error("Non-Integer Argument For Integer Format");
            }

            template<class R, class T>
            constexpr R formatted_float(T n, typename std::enable_if<
                    std::is_floating_point<T>::value || std::is_integral<T>::value>::type * = 0) {
                return static_cast<R>(n);
            }

            template<class R, class T>
            R formatted_float(T n, typename std::enable_if<
                    !std::is_floating_point<T>::value && !std::is_integral<T>::value>::type * = 0) {
                (void) n;
                throw format_error("Non-Floating_point Argument for Floating_point Format");
            }

//------------------------------------------------------------------------------
// Name: process_format
// Desc: default handler that should never be called at runtime
//------------------------------------------------------------------------------
            template<class Context>
            int process_format(Context &ctx, const char *format, Flags flags, long int width, long int precision,
                               Modifiers modifier) {
                (void) ctx;
                (void) format;
                (void) flags;
                (void) width;
                (void) precision;
                (void) modifier;
                throw format_error("Should Never Happen");
            }

//------------------------------------------------------------------------------
// Name: get_modifier
// Desc: default handler that should never be called at runtime
//------------------------------------------------------------------------------
            template<class Context>
            int get_modifier(Context &ctx, const char *format, Flags flags, long int width, long int precision) {
                (void) ctx;
                (void) format;
                (void) flags;
                (void) width;
                (void) precision;
                throw format_error("Should Never Happen");
            }

//------------------------------------------------------------------------------
// Name: get_precision
// Desc: default handler that should never be called at runtime
//------------------------------------------------------------------------------
            template<class Context>
            int get_precision(Context &ctx, const char *format, Flags flags, long int width) {
                (void) ctx;
                (void) format;
                (void) flags;
                (void) width;
                throw format_error("Should Never Happen");
            }

//------------------------------------------------------------------------------
// Name: process_format
// Desc: prints the next argument to the Context taking into account the flags,
//       width, precision, and modifiers collected along the way. Then will
//       recursively continue processing the string
//------------------------------------------------------------------------------
            template<class Context, class T, class... Ts>
            int process_format(Context &ctx, const char *format, Flags flags, int32_t width, int32_t precision,
                               Modifiers modifier, const T &arg, const Ts &... ts) {

                // enough to contain a 128-bit number in bin notation + optional prefix
                char num_buf[131];

                size_t slen;
                const char *s_ptr = nullptr;

                char ch = *format;
                switch (ch) {
                    case 'a':
                    case 'A':
                        // TODO (farresti): implement hexa formatting for float numbers.. for now just consume argument.
                        return Printf(ctx, format + 1, ts...);
                    case 'e':
                    case 'E':
                    case 'f':
                    case 'F':
                    case 'g':
                    case 'G':
                        if (precision < 0) {
                            /* == Default floating_point precision == */
                            precision = 6;
                        }
                        switch (modifier) {
                            case Modifiers::MOD_LONG_DOUBLE:
                                s_ptr = ftoa(num_buf, ch, precision, formatted_float<long double>(arg),
                                             width,
                                             flags, &slen);
                                break;
                            case Modifiers::MOD_LONG:
                            default:
                                s_ptr = ftoa(num_buf, ch, precision, formatted_float<double>(arg),
                                             width,
                                             flags, &slen);
                                break;
                        }
                        output_string(ch, s_ptr, precision, width, flags, static_cast<int32_t>(slen), ctx);
                        return Printf(ctx, format + 1, ts...);

                    case 'p':
                        precision = 1;
                        ch = 'x';
                        flags.prefix = 1;
                        // NOTE(eteran): GNU printf prints "(nil)" for NULL pointers, we print 0x0
                        s_ptr = itoa(num_buf, ch, precision, formatted_pointer<uintptr_t>(arg), width, flags, &slen);

                        output_string(ch, s_ptr, precision, width, flags, static_cast<int32_t>(slen), ctx);
                        return Printf(ctx, format + 1, ts...);

                    case 'x':
                    case 'X':
                    case 'u':
                    case 'o':
#ifdef CXX11_PRINTF_EXTENSIONS
                    case 'b': // extension, BINARY mode
#endif
                        if (precision < 0) {
                            precision = 1;
                        }

                        switch (modifier) {
                            case Modifiers::MOD_CHAR:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<unsigned char>(arg), width,
                                             flags,
                                             &slen);
                                break;
                            case Modifiers::MOD_SHORT:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<unsigned short int>(arg), width,
                                             flags, &slen);
                                break;
                            case Modifiers::MOD_LONG:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<unsigned long int>(arg), width,
                                             flags, &slen);
                                break;
                            case Modifiers::MOD_LONG_LONG:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<unsigned long long int>(arg),
                                             width,
                                             flags, &slen);
                                break;
                            case Modifiers::MOD_INTMAX_T:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<uintmax_t>(arg), width, flags,
                                             &slen);
                                break;
                            case Modifiers::MOD_SIZE_T:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<size_t>(arg), width, flags,
                                             &slen);
                                break;
                            case Modifiers::MOD_PTRDIFF_T:
                                s_ptr = itoa(num_buf, ch, precision,
                                             formatted_integer<std::make_unsigned<ptrdiff_t>::type>(arg), width, flags,
                                             &slen);
                                break;
                            default:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<unsigned int>(arg), width, flags,
                                             &slen);
                                break;
                        }

                        output_string(ch, s_ptr, precision, width, flags, static_cast<int32_t>(slen), ctx);
                        return Printf(ctx, format + 1, ts...);

                    case 'i':
                    case 'd':
                        if (precision < 0) {
                            precision = 1;
                        }

                        switch (modifier) {
                            case Modifiers::MOD_CHAR:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<signed char>(arg), width, flags,
                                             &slen);
                                break;
                            case Modifiers::MOD_SHORT:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<short int>(arg), width, flags,
                                             &slen);
                                break;
                            case Modifiers::MOD_LONG:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<long int>(arg), width, flags,
                                             &slen);
                                break;
                            case Modifiers::MOD_LONG_LONG:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<long long int>(arg), width,
                                             flags,
                                             &slen);
                                break;
                            case Modifiers::MOD_INTMAX_T:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<intmax_t>(arg), width, flags,
                                             &slen);
                                break;
                            case Modifiers::MOD_SIZE_T:
                                s_ptr = itoa(num_buf, ch, precision,
                                             formatted_integer<std::make_signed<size_t>::type>(arg),
                                             width, flags, &slen);
                                break;
                            case Modifiers::MOD_PTRDIFF_T:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<ptrdiff_t>(arg), width, flags,
                                             &slen);
                                break;
                            default:
                                s_ptr = itoa(num_buf, ch, precision, formatted_integer<int>(arg), width, flags, &slen);
                                break;
                        }

                        output_string(ch, s_ptr, precision, width, flags, static_cast<int32_t>(slen), ctx);
                        return Printf(ctx, format + 1, ts...);

                    case 'c':
                        // char is promoted to an int when pushed on the stack
                        num_buf[0] = formatted_integer<char>(arg);
                        num_buf[1] = '\0';
                        s_ptr = num_buf;
                        output_string('c', s_ptr, precision, width, flags, 1, ctx);
                        return Printf(ctx, format + 1, ts...);

                    case 's':
                        s_ptr = formatted_string(arg);
                        if (!s_ptr) {
                            s_ptr = "(null)";
                        }
                        output_string('s', s_ptr, precision, width, flags, static_cast<int32_t>(strlen(s_ptr)), ctx);
                        return Printf(ctx, format + 1, ts...);

#ifdef CXX11_PRINTF_EXTENSIONS
                    case '?': {
                        std::string s = formatted_object(arg);
                        output_string('s', s.data(), precision, width, flags, static_cast<int32_t>(s.size()), ctx);
                    }
                        return Printf(ctx, format + 1, ts...);
#endif

                    case 'n':
                        switch (modifier) {
                            case Modifiers::MOD_CHAR:
                                *formatted_pointer<signed char *>(arg) = static_cast<signed char>(ctx.written);
                                break;
                            case Modifiers::MOD_SHORT:
                                *formatted_pointer<short int *>(arg) = static_cast<short int>(ctx.written);
                                break;
                            case Modifiers::MOD_LONG:
                                *formatted_pointer<long int *>(arg) = static_cast<long int>(ctx.written);
                                break;
                            case Modifiers::MOD_LONG_LONG:
                                *formatted_pointer<long long int *>(arg) = static_cast<long long int>(ctx.written);
                                break;
                            case Modifiers::MOD_INTMAX_T:
                                *formatted_pointer<intmax_t *>(arg) = static_cast<intmax_t>(ctx.written);
                                break;
                            case Modifiers::MOD_SIZE_T:
                                *(formatted_pointer<std::make_signed<size_t>::type *>(
                                        arg)) = static_cast<std::make_signed<size_t>::type>(ctx.written);
                                break;
                            case Modifiers::MOD_PTRDIFF_T:
                                *(formatted_pointer<ptrdiff_t *>(arg)) = static_cast<ptrdiff_t>(ctx.written);
                                break;
                            default:
                                *(formatted_pointer<int *>(arg)) = static_cast<int32_t>(ctx.written);
                                break;
                        }

                        return Printf(ctx, format + 1, ts...);

                    default:
                        ctx.write('%');
                        // FALL THROUGH
                    case '\0':
                    case '%':
                        ctx.write(ch);
                        break;
                }

                return Printf(ctx, format + 1, ts...);
            }

//------------------------------------------------------------------------------
// Name: get_modifier
// Desc: gets the modifier, if any, from the format string, then calls
//       process_format
//------------------------------------------------------------------------------
            template<class Context, class T, class... Ts>
            int32_t
            get_modifier(Context &ctx, const char *format, Flags flags, int32_t width, int32_t precision,
                         const T &arg,
                         const Ts &... ts) {


                Modifiers modifier = Modifiers::MOD_NONE;

                switch (*format) {
                    case 'h':
                        modifier = Modifiers::MOD_SHORT;
                        ++format;
                        if (*format == 'h') {
                            modifier = Modifiers::MOD_CHAR;
                            ++format;
                        }
                        break;
                    case 'l':
                        modifier = Modifiers::MOD_LONG;
                        ++format;
                        if (*format == 'l') {
                            modifier = Modifiers::MOD_LONG_LONG;
                            ++format;
                        }
                        break;
                    case 'L':
                        modifier = Modifiers::MOD_LONG_DOUBLE;
                        ++format;
                        break;
                    case 'j':
                        modifier = Modifiers::MOD_INTMAX_T;
                        ++format;
                        break;
                    case 'z':
                        modifier = Modifiers::MOD_SIZE_T;
                        ++format;
                        break;
                    case 't':
                        modifier = Modifiers::MOD_PTRDIFF_T;
                        ++format;
                        break;
                    default:
                        break;
                }

                return process_format(ctx, format, flags, width, precision, modifier, arg, ts...);
            }

//------------------------------------------------------------------------------
// Name: get_precision
// Desc: gets the precision, if any, either from the format string or as an arg
//       as needed, then calls get_modifier
//------------------------------------------------------------------------------
            template<class Context, class T, class... Ts>
            int32_t
            get_precision(Context &ctx, const char *format, Flags flags, int32_t width, const T &arg,
                          const Ts &... ts) {

                // default to non-existant
                int32_t p = -1;

                if (*format == '.') {

                    ++format;
                    if (*format == '*') {
                        ++format;
                        // pull an int off the stack for processing
                        p = formatted_integer<int32_t>(arg);
                        return get_modifier(ctx, format, flags, width, p, ts...);
                    } else {
                        char *endptr;
                        p = static_cast<int32_t>(strtol(format, &endptr, 10));
                        format = endptr;
                        return get_modifier(ctx, format, flags, width, p, arg, ts...);
                    }
                }

                return get_modifier(ctx, format, flags, width, p, arg, ts...);
            }

//------------------------------------------------------------------------------
// Name: get_width
// Desc: gets the width if any, either from the format string or as an arg as
//       needed, then calls get_precision
//------------------------------------------------------------------------------
            template<class Context, class T, class... Ts>
            int get_width(Context &ctx, const char *format, Flags flags, const T &arg, const Ts &... ts) {

                int width = 0;

                if (*format == '*') {
                    ++format;
                    // pull an int off the stack for processing
                    width = formatted_integer<int32_t>(arg);

                    return get_precision(ctx, format, flags, width, ts...);
                } else {
                    char *endptr;
                    width = static_cast<int32_t>(strtol(format, &endptr, 10));
                    format = endptr;

                    return get_precision(ctx, format, flags, width, arg, ts...);
                }
            }

//------------------------------------------------------------------------------
// Name: get_flags
// Desc: gets the flags, if any, from the format string, then calls get_width
//------------------------------------------------------------------------------
            template<class Context, class... Ts>
            int get_flags(Context &ctx, const char *format, const Ts &... ts) {

                Flags f = { 0, 0, 0, 0, 0, 0 };
                bool done = false;

                // skip past the % char
                ++format;

                while (!done) {

                    char ch = *format++;

                    switch (ch) {
                        case '-':
                            // justify, overrides padding
                            f.justify = 1;
                            f.padding = 0;
                            break;
                        case '+':
                            // sign, overrides space
                            f.sign = 1;
                            f.space = 0;
                            break;
                        case ' ':
                            if (!f.sign) {
                                f.space = 1;
                            }
                            break;
                        case '#':
                            f.prefix = 1;
                            break;
                        case '0':
                            if (!f.justify) {
                                f.padding = 1;
                            }
                            break;
                        default:
                            done = true;
                            --format;
                    }
                }

                return get_width(ctx, format, f, ts...);
            }
        }

//------------------------------------------------------------------------------
// Name: Printf
// Desc: 0 argument version of Printf. Asserts on any format character found
//------------------------------------------------------------------------------
        template<class Context>
        int Printf(Context &ctx, const char *format) {

            assert(format);

            for (; *format; ++format) {
                if (*format != '%' || *++format == '%') {
                    ctx.write(*format);
                    continue;
                }

                throw format_error("Bad Format");
            }

            // this will usually null terminate the string
            ctx.done();

            // return the amount of bytes that should have been written if there was sufficient space
            return static_cast<int>(ctx.written);
        }

//------------------------------------------------------------------------------
// Name: Printf
// Desc: 1+ argument version of Printf.
//------------------------------------------------------------------------------
        template<class Context, class... Ts>
        int Printf(Context &ctx, const char *format, const Ts &... ts) {
            assert(format);
            constexpr auto n_args = sizeof...(ts);
            while (*format != '\0') {
                if (*format == '%' && *++format != '%') {
                    --format;
                    // %[flag][width][.precision][length]char
                    // this recurses into get_width -> get_precision -> get_length -> process_format
                    return detail::get_flags(ctx, format, ts...);
                } else {
                    ctx.write(*format);
                }
                ++format;
            }
            if (n_args) {
                throw format_error("Bad format: expected format_specifier.");
            }
            // clean up any trailing stuff
            return Printf(ctx, format + 1, ts...);
        }

//------------------------------------------------------------------------------
// Name: snprintf
// Desc: implementation of what snprintf compatible interface
//------------------------------------------------------------------------------
        template<class... Ts>
        int sprintf(std::ostream &os, const char *format, const Ts &... ts) {
            ostream_writer ctx(os);
            return Printf(ctx, format, ts...);
        }

//------------------------------------------------------------------------------
// Name: sprintf
// Desc: implementation of what s[n]printf compatible interface
//------------------------------------------------------------------------------
        template<class... Ts>
        int sprintf(char *str, size_t size, const char *format, const Ts &... ts) {
            buffer_writer ctx(str, size);
            return Printf(ctx, format, ts...);
        }

//------------------------------------------------------------------------------
// Name: printf
// Desc: implementation of what printf compatible interface
//------------------------------------------------------------------------------
        template<class... Ts>
        int printf(const char *format, const Ts &... ts) {
            stdout_writer ctx;
            return Printf(ctx, format, ts...);
        }

//------------------------------------------------------------------------------
// Name: fprintf
// Desc: implementation of what fprintf compatible interface
//------------------------------------------------------------------------------
        template<class... Ts>
        int fprintf(FILE *stream, const char *format, const Ts &... ts) {
            stdio_writer ctx(stream);
            return Printf(ctx, format, ts...);
        }
    }
}
#endif
