/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2014 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2014 - 2018)
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
#ifndef SPIDER2_LOGGER_H
#define SPIDER2_LOGGER_H

/* === Includes === */

#include <cstdint>
#include <cstdio>
#include <mutex>
#include <common/Printer.h>
#include <common/Exception.h>
#include <spider-api/config.h>
#include <spider-api/debug.h>

/* === Namespace === */

namespace spider {
    namespace log {

        constexpr static const char green[] = "\x1B[32m";
        constexpr static const char red[] = "\x1B[31m";
        constexpr static const char yellow[] = "\x1B[33m";
        constexpr static const char blue[] = "\x1B[34m";
        constexpr static const char magenta[] = "\x1B[35m";
        constexpr static const char cyan[] = "\x1B[36m";
        constexpr static const char white[] = "\x1B[37m";
        constexpr static const char normal[] = "\x1B[0m";

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

        template<log::Type type>
        inline constexpr log::Log &logger() {
            return loggers().at(type);
        }

        template<log::Type type>
        inline void enable() {
            std::lock_guard<std::mutex> locker(mutex());
            logger<type>().enabled_ = true;
        }

        template<log::Type type>
        inline void disable() {
            std::lock_guard<std::mutex> locker(mutex());
            logger<type>().enabled_ = false;
        }

        template<log::Type type, const char color[], const char level[], class... Args>
        inline void print(const char *fmt, Args &&... args) {
            std::lock_guard<std::mutex> locker(mutex());
            spider::printer::fprintf(outputStream(), "%s[%s:%s]:", color, logger<type>().litteral_, level);
            spider::printer::fprintf(outputStream(), fmt, std::forward<Args>(args)...);
            spider::printer::fprintf(outputStream(), normal);
        }

        /**
         * @brief Print information.
         * @tparam Args  Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<log::Type type = log::Type::GENERAL, class... Args>
        inline void info(const char *fmt, Args &&...args) {
            constexpr static const char lvl[] = "INFO";
            print<type, white, lvl>(fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Print non-critical information. However, these should be looked up as they indicate mis-behavior.
         * @tparam Args    Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<log::Type type = log::Type::GENERAL, class... Args>
        inline void warning(const char *fmt, Args &&...args) {
            constexpr static const char lvl[] = "WARN";
            print<type, yellow, lvl>(fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Print critical information. Usually application will fail after.
         * @tparam Args    Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<log::Type type = log::Type::GENERAL, class... Args>
        inline void error(const char *fmt, Args &&...args) {
            constexpr static const char lvl[] = "ERR";
            print<type, red, lvl>(fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Print information only when using the verbose mode of spider.
         * @tparam Args    Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<log::Type type = log::Type::GENERAL, class... Args>
        inline void verbose(const char *fmt, Args &&...args) {
            constexpr static const char lvl[] = "VERB";
            print<type, green, lvl>(fmt, std::forward<Args>(args)...);
        }
    }
}

/* === Aliases for logger === */

constexpr auto LOG_LRT = spider::log::Type::LRT;
constexpr auto LOG_TIME = spider::log::Type::TIME;
constexpr auto LOG_GENERAL = spider::log::Type::GENERAL;
constexpr auto LOG_MEMORY = spider::log::Type::MEMORY;
constexpr auto LOG_SCHEDULE = spider::log::Type::SCHEDULE;
constexpr auto LOG_TRANSFO = spider::log::Type::TRANSFO;
constexpr auto LOG_OPTIMS = spider::log::Type::OPTIMS;
constexpr auto LOG_EXPR = spider::log::Type::EXPR;

template<spider::log::Type type = spider::log::Type::GENERAL>
inline constexpr bool log_enabled() {
    return spider::log::logger<type>().enabled_;
}

#endif // SPIDER2_LOGGER_H
