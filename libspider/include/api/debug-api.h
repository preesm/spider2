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
#ifndef SPIDER2_DEBUG_API_H
#define SPIDER2_DEBUG_API_H

/* === Includes === */

#include <api/global-api.h>
#include <array>

namespace spider {

    /* === const-array(s) === */

    inline std::array<log::Log, log::LOGGER_COUNT> &loggers() {
        static std::array<log::Log, log::LOGGER_COUNT> loggerArray = {{{ "LRT", false },
                                                                              { "TIME", false },
                                                                              { "GENERAL", false },
                                                                              { "SCHEDULE", false },
                                                                              { "MEMORY", false },
                                                                              { "TRANSFO", false },
                                                                              { "OPTIMS", false },
                                                                              { "EXPR", false }}};
        return loggerArray;
    }

    namespace api {

        /* === Function(s) prototype === */

        /**
         * @brief Export a PiSDF graph to a .dot file.
         * @param path   Path of the file.
         * @param graph  Graph to transform (default is the application graph).
         */
        void exportGraphToDOT(pisdf::Graph *graph = nullptr, const std::string &path = "./graph.dot");

        /**
         * @brief Enable a given logger.
         * @param type @refitem LoggerType to enable.
         */
        void enableLogger(log::Type type);

        /**
         * @brief Disable a given logger.
         * @param type @refitem LoggerType to disable.
         */
        void disableLogger(log::Type type);

        /**
         * @brief Set the logger output stream (default is stderr).
         * @param stream Pointer to the stream to use.
         */
        void setLoggerSteam(FILE *stream);
    }
}

#endif //SPIDER2_DEBUG_API_H
