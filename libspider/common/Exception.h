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
#ifndef SPIDER2_EXCEPTION_H
#define SPIDER2_EXCEPTION_H

/* === Includes === */

#include <cstring>
#include <stdexcept>
#include <common/Printer.h>

/* === Defines === */

/* == Size of 50 minimum is required for the error message associated == */
constexpr auto EXCEPTION_BUFFER_SIZE = 400;

/* === Macros === */

#define throwSpiderException(...) throw spider::Exception(spider::getFileName(__FILE__), __func__, __LINE__, __VA_ARGS__)

#define throwNullptrException() throw spider::Exception(spider::getFileName(__FILE__), __func__, __LINE__, "nullptr exception")

namespace spider {

    constexpr const char *str_end(const char *str) {
        return *str ? str_end(str + 1) : str;
    }

    constexpr bool str_slant(const char *str) {
        return *str == '/' || (*str != 0 && str_slant(str + 1));
    }

    constexpr const char *r_slant(const char *str) {
        return *str == '/' ? (str + 1) : r_slant(str - 1);
    }

    constexpr const char *getFileName(const char *str) {
        return str_slant(str) ? r_slant(str_end(str)) : str;
    }

    /* === Class definition === */

    class Exception : public std::exception {
    public:
        template<class... Args>
        explicit Exception(const char *fileName, const char *fctName, int lineNumber,
                           const char *msg, Args &&...args)
                : exceptionMessage_{ } {
            /* == Writes exception header == */
            int n = printer::snprintf(exceptionMessage_, EXCEPTION_BUFFER_SIZE, "%s::%s(%d): ", fileName,
                                      fctName, lineNumber);

            /* == Write the actual exception message == */
            n = printer::snprintf(exceptionMessage_ + n, static_cast<size_t>(EXCEPTION_BUFFER_SIZE - n), msg,
                                  std::forward<Args>(args)...);
            if (n > EXCEPTION_BUFFER_SIZE) {
                printer::fprintf(stderr, "Exception: ERROR: exception message too big.\n");
                printer::fprintf(stderr, "Partially recovered exception: %s\n", exceptionMessage_);
                fflush(stderr);
            }
        }

        const char *what() const noexcept override { return exceptionMessage_; }

    private:
        char exceptionMessage_[EXCEPTION_BUFFER_SIZE];
    };
}
#endif //SPIDER2_EXCEPTION_H
