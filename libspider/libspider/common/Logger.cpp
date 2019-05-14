/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2014 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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

/* === Includes === */

#include <mutex>
#include <cstdarg>
#include <common/Logger.h>
#include <common/memory/Allocator.h>

/* === Defines === */

#define LOG_RED "\x1B[31m"
#define LOG_GRN "\x1B[32m"
#define LOG_YEL "\x1B[33m"
#define LOG_BLU "\x1B[34m"
#define LOG_MAG "\x1B[35m"
#define LOG_CYN "\x1B[36m"
#define LOG_WHT "\x1B[37m"
#define LOG_NRM "\x1B[0m"

/* === Static variables === */

static bool loggersState_[N_LOGGER];

static FILE *outStream_ = stderr;

static const char *loggersLitteral[N_LOGGER] = {
        "JOB",
        "TIME",
        "GENERAL",
};

static std::mutex lock;

/* === Methods implementation === */

void Logger::init() {
    loggersState_[LOG_JOB] = false;
    loggersState_[LOG_TIME] = false;
    loggersState_[LOG_GENERAL] = true;
}

void Logger::enable(LoggerType type) {
    std::lock_guard<std::mutex> locker(lock);
    loggersState_[type] = true;
}

void Logger::disable(LoggerType type) {
    std::lock_guard<std::mutex> locker(lock);
    loggersState_[type] = false;
}

void Logger::setOutputStream(FILE *stream) {
    std::lock_guard<std::mutex> locker(lock);
    outStream_ = stream;
}

void Logger::print(LoggerType type, LoggerLevel level, const char *fmt, ...) {
    if (loggersState_[type]) {
        std::lock_guard<std::mutex> locker(lock);
        va_list l;
        va_start(l, fmt);
        fprintf(outStream_, "%s-", loggersLitteral[type]);
        switch (level) {
            case LOG_INFO:
                fprintf(outStream_, LOG_NRM "INFO: ");
                break;
            case LOG_WARNING:
                fprintf(outStream_, LOG_YEL "WARNING: ");
                break;
            case LOG_ERROR:
                fprintf(outStream_, LOG_RED "ERROR: ");
                break;
            default:
                fprintf(outStream_, LOG_GRN "UNDEFINED: ");
                break;
        }
        vfprintf(outStream_, fmt, l);
        fprintf(outStream_, LOG_NRM);
        va_end(l);
    }
}