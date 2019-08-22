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
#include <common/cxx11-printf/Printf.h>
#include <common/SpiderException.h>
#include <spider-api/general.h>
#include <spider-api/debug.h>

/* === Define(s) === */

#define N_LOGGER 7
#define LOG_RED "\x1B[31m"
#define LOG_GRN "\x1B[32m"
#define LOG_YEL "\x1B[33m"
#define LOG_BLU "\x1B[34m"
#define LOG_MAG "\x1B[35m"
#define LOG_CYN "\x1B[36m"
#define LOG_WHT "\x1B[37m"
#define LOG_NRM "\x1B[0m"

/* === Namespace === */

namespace Spider {
    namespace Logger {

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

        inline bool &logger(std::uint8_t ix) {
            static bool loggers[N_LOGGER] = {false};
            return loggers[ix];
        }

        inline void enable(LoggerType type) {
            if (type >= N_LOGGER || type < 0) {
                throwSpiderException("Invalid logger type: %d", type);
            }
            std::lock_guard<std::mutex> locker(mutex());
            logger(type) = true;
        }

        inline void disable(LoggerType type) {
            if (type >= N_LOGGER || type < 0) {
                throwSpiderException("Invalid logger type: %d", type);
            }
            std::lock_guard<std::mutex> locker(mutex());
            logger(type) = false;
        }

        template<class... Ts>
        inline void print(LoggerType type,
                          const char *colorCode,
                          const char *level,
                          const char *fmt, const Ts &... ts) {
            constexpr const char *loggersLiteral[N_LOGGER] = {
                    "LRT",
                    "TIME",
                    "GENERAL",
                    "SCHEDULE",
                    "MEMORY",
                    "TRANSFO",
                    "OPTIMS",
            };
            if (logger(type)) {
                std::lock_guard<std::mutex> locker(mutex());
                Spider::cxx11::fprintf(outputStream(), "%s[%s:%s]:", colorCode, loggersLiteral[type], level);
                Spider::cxx11::fprintf(outputStream(), fmt, ts...);
                Spider::cxx11::fprintf(outputStream(), LOG_NRM);
            }
        }

        /**
         * @brief Print information.
         * @tparam Ts    Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<class... Ts>
        inline void printInfo(LoggerType type, const char *fmt, const Ts &...ts) {
            print(type, LOG_NRM, "INFO", fmt, ts...);
        }

        /**
         * @brief Print non-critical information. However, these should be looked up as they indicate mis-behavior.
         * @tparam Ts    Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<class... Ts>
        inline void printWarning(LoggerType type, const char *fmt, const Ts &...ts) {
            print(type, LOG_YEL, "WARN", fmt, ts...);
        }

        /**
         * @brief Print critical information. Usually application will fail after.
         * @tparam Ts    Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<class... Ts>
        inline void printError(LoggerType type, const char *fmt, const Ts &...ts) {
            print(type, LOG_RED, "ERR ", fmt, ts...);
        }

        /**
         * @brief Print information only when using the verbose mode of spider.
         * @tparam Ts    Variadic parameters.
         * @param type   @refitem LoggerType -> type of the logger.
         * @param fmt    Formatted string to print.
         * @param ts     Arguments to be printed.
         */
        template<class... Ts>
        inline void printVerbose(LoggerType type, const char *fmt, const Ts &...ts) {
            if (Spider::API::verbose()) {
                print(type, LOG_GRN, "VERB", fmt, ts...);
            }
        }
    }
}

#endif // SPIDER2_LOGGER_H
