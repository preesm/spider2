/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
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
#ifndef SPIDER2_LOGGER_H
#define SPIDER2_LOGGER_H

/* === Includes === */

#include <cstdint>
#include <cstdio>
#include <mutex>
#include <common/Printer.h>
#include <common/Exception.h>
#include <api/debug-api.h>

/* === Namespace === */

namespace spider {
    namespace log {

        constexpr static auto green = "\x1B[32m";
        constexpr static auto red = "\x1B[31m";
        constexpr static auto yellow = "\x1B[33m";
        constexpr static auto blue = "\x1B[34m";
        constexpr static auto magenta = "\x1B[35m";
        constexpr static auto cyan = "\x1B[36m";
        constexpr static auto white = "\x1B[37m";
        constexpr static auto normal = "\x1B[0m";

        inline std::mutex &mutex() {
            static std::mutex lock;
            return lock;
        }

        inline FILE *&outputStream() {
            static FILE *stream = stderr;
            return stream;
        }

        inline void setOutputStream(FILE *stream) {
            std::lock_guard<std::mutex> locker(mutex());
            outputStream() = stream;
        }

        template<Type type>
        inline constexpr log::Log &logger() {
            return loggers().at(type);
        }

        template<Type type>
        inline void enable() {
            std::lock_guard<std::mutex> locker(mutex());
            logger<type>().enabled_ = true;
        }

        template<Type type>
        inline void disable() {
            std::lock_guard<std::mutex> locker(mutex());
            logger<type>().enabled_ = false;
        }

        template<Type type, class... Args>
        inline void print(const char color[], const char level[], const char *fmt, Args &&... args) {
            std::lock_guard<std::mutex> locker(mutex());
            printer::fprintf(outputStream(), "%s[%s:%s]:", color, logger<type>().litteral_, level);
            printer::fprintf(outputStream(), fmt, std::forward<Args>(args)...);
            printer::fprintf(outputStream(), normal);
        }

        /**
         * @brief Print information.
         * @tparam Args  Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<Type type = Type::GENERAL, class... Args>
        inline void info(const char *fmt, Args &&...args) {
            constexpr static const char lvl[] = "INFO";
            print<type>(white, lvl, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Print non-critical information. However, these should be looked up as they indicate mis-behavior.
         * @tparam Args    Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<Type type = Type::GENERAL, class... Args>
        inline void warning(const char *fmt, Args &&...args) {
            constexpr static const char lvl[] = "WARN";
            print<type>(yellow, lvl, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Print critical information. Usually application will fail after.
         * @tparam Args    Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<Type type = Type::GENERAL, class... Args>
        inline void error(const char *fmt, Args &&...args) {
            constexpr static const char lvl[] = "ERR";
            print<type>(red, lvl, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Print information only when using the verbose mode of spider.
         * @tparam Args    Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<Type type = Type::GENERAL, class... Args>
        inline void verbose(const char *fmt, Args &&...args) {
            constexpr static const char lvl[] = "VERB";
            print<type>(green, lvl, fmt, std::forward<Args>(args)...);
        }

        template<spider::log::Type type = spider::log::GENERAL>
        inline constexpr bool enabled() {
            return spider::log::logger<type>().enabled_;
        }
    }
}
#endif // SPIDER2_LOGGER_H
