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
 * Slight modifications have been done in order to uniformize with the rest of the Spider code base.
 */
#ifndef FORMATTERS_20160922_H_
#define FORMATTERS_20160922_H_

#include <cstddef>
#include <cstring>
#include <iterator>

namespace Spider {
    namespace cxx11 {

        /**
         * @brief This context writes to a buffer.
         */
        struct buffer_writer {

            buffer_writer(char *buffer, size_t size) : ptr_(buffer), size_(size) {
            }

            void write(char ch) noexcept {
                if (size_ > 1) {
                    *ptr_++ = ch;
                    --size_;
                }
                ++written;
            }

            void write(const char *p, size_t n) {
                size_t count = std::min(size_, n);
                memcpy(ptr_, p, count);
                ptr_ += count;
                size_ -= count;
                written += count;
            }

            void done() noexcept {
                if (size_ != 0) {
                    *ptr_ = '\0';
                }
            }

            char *ptr_;
            size_t size_;
            size_t written = 0;
        };

        /**
         * @brief This context writes to a container using a std::back_inserter.
         */
        struct ostream_writer {

            explicit ostream_writer(std::ostream &os) : os_(os) {
            }

            void write(char ch) {
                os_.put(ch);
                ++written;
            }

            void write(const char *p, size_t n) {
                while (n--) {
                    write(*p++);
                }
            }

            static void done() noexcept { }

            std::ostream &os_;
            size_t written = 0;
        };

        /**
         * @brief Context that writes to a container using a std::back_inserter
         * @tparam C Container type to write to.
         */
        template<class C>
        struct container_writer {
            explicit container_writer(C &s) : it_(std::back_inserter(s)) {
            }

            void write(char ch) {
                *it_++ = ch;
                ++written;
            }

            void write(const char *p, size_t n) {
                while (n--) {
                    write(*p++);
                }
            }

            static void done() noexcept { }

            std::back_insert_iterator<C> it_;
            size_t written = 0;
        };

        /**
         * @brief Context to write to an stdio stream
         */
        struct stdio_writer {
            explicit stdio_writer(FILE *stream) : stream_(stream) {
            }

            void write(char ch) noexcept {
                putc(ch, stream_);
                ++written;
            }

            void write(const char *p, size_t n) {
                while (n--) {
                    write(*p++);
                }
            }

            static void done() noexcept { }

            FILE *stream_;
            size_t written = 0;
        };

        /**
         * @brief Context to write to stdout stream
         */
        struct stdout_writer final : public stdio_writer {
            stdout_writer() : stdio_writer(stdout) {

            }
        };

        /**
         * @brief Context to write to stderr stream
         */
        struct stderr_writer final : public stdio_writer {
            stderr_writer() : stdio_writer(stderr) {

            }
        };
    }
}
#endif
