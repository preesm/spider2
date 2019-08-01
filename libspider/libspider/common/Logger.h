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
#include <common/cxx11-printf/printf.h>

/* === Define(s) === */

#define N_LOGGER 4
#define LOG_RED "\x1B[31m"
#define LOG_GRN "\x1B[32m"
#define LOG_YEL "\x1B[33m"
#define LOG_BLU "\x1B[34m"
#define LOG_MAG "\x1B[35m"
#define LOG_CYN "\x1B[36m"
#define LOG_WHT "\x1B[37m"
#define LOG_NRM "\x1B[0m"

/* === Enumeration(s) === */

typedef enum {
    LOG_JOB = 0,
    LOG_TIME = 1,
    LOG_GENERAL = 2,
} LoggerType;

/* === Namespace === */

namespace Spider {
    namespace Logger {

        void init();

        void enable(LoggerType type);

        void disable(LoggerType type);

        void setOutputStream(FILE *stream);

        FILE *&outputStream();

        std::mutex &mutex();

        bool loggerEnabled(std::uint8_t ix);

        template<class... Ts>
        inline void print(LoggerType type, const char *colorCode, const char *fmt, const Ts &... ts) {
            constexpr const char *loggersLiteral[N_LOGGER] = {
                    "JOB",
                    "TIME",
                    "GENERAL",
                    "VERBOSE",
            };
            if (loggerEnabled(type)) {
                std::lock_guard<std::mutex> locker(mutex());
                Spider::cxx11::fprintf(outputStream(), "%s%s-", loggersLiteral[type], colorCode);
                Spider::cxx11::fprintf(outputStream(), fmt, ts...);
                Spider::cxx11::fprintf(outputStream(), LOG_NRM);
            }
        }

        template<class... Ts>
        inline void printInfo(LoggerType type, const char *fmt, const Ts &...ts) {
            print(type, LOG_NRM, fmt, ts...);
        }

        template<class... Ts>
        inline void printWarning(LoggerType type, const char *fmt, const Ts &...ts) {
            print(type, LOG_YEL, fmt, ts...);
        }

        template<class... Ts>
        inline void printError(LoggerType type, const char *fmt, const Ts &...ts) {
            print(type, LOG_RED, fmt, ts...);
        }

        template<class... Ts>
        inline void printVerbose(LoggerType type, const char *fmt, const Ts &...ts) {
            print(type, LOG_GRN, fmt, ts...);
        }
    }
}

#endif // SPIDER2_LOGGER_H
