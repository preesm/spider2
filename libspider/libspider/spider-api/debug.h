/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
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
#ifndef SPIDER2_DEBUG_H
#define SPIDER2_DEBUG_H

/* === Includes === */

#include <spider-api/pisdf.h>
#include <array>

namespace Spider {

    /* === Enumeration(s) === */

    namespace Logger {
        enum class Type : std::uint8_t {
            LRT,        /*! LRT logger. When enabled, this will print LRT logged information. */
            TIME,       /*! TIME logger. When enabled this will print time logged information */
            GENERAL,    /*! GENERAL purpose logger, used for information about almost everything */
            SCHEDULE,   /*! SCHEDULE logger. When enabled, this will print Schedule logged information. */
            MEMORY,     /*! MEMORY logger. When enabled, this will print Memory logged information. */
            TRANSFO,    /*! TRANSFO logger. When enabled, this will print transformation logged information. */
            OPTIMS,     /*! OPTIMS logger. When enabled, this will print transformation logged information. */
            EXPR,       /*! EXPRESSION logger. When enabled, this will print expression-parser logged information. */
        };

        class Log {
        public:
            const char *litteral_;
            bool enabled_;
        };

        constexpr auto loggerCount = static_cast<std::uint8_t >(Type::EXPR) + 1;

        inline std::array<Log, loggerCount> &loggers() {
            static std::array<Log, loggerCount> loggerArray = {{{ "LRT", false },
                                                                       { "TIME", false },
                                                                       { "GENERAL", false },
                                                                       { "SCHEDULE", false },
                                                                       { "MEMORY", false },
                                                                       { "TRANSFO", false },
                                                                       { "OPTIMS", false },
                                                                       { "EXPR", false }}};
            return loggerArray;
        }
    }

    namespace API {

        /* === Function(s) prototype === */

        /**
         * @brief Export the Gantt of the real execution trace of the application for 1 graph iteration.
         * @remark Requires to have enable the execution traces with @refitem Spider::enableTrace.
         * @param path Path of the file.
         */
        void exportPostExecGantt(const std::string &path);

        /**
         * @brief Export the expected Gantt obtained by the scheduling algorithm.
         * @param path Path of the file.
         */
        void exportPreExecGantt(const std::string &path);

        /**
         * @brief Export the equivalent Single-Rate Directed Acyclic Graph (SR-DAG) of a graph
         * after one graph iteration to a .dot file.
         * @remark This function consider that dynamic parameters have been resolved.
         * @remark This function perform the SR-DAG transformation of the graph.
         * @param path   Path of the file.
         * @param graph  Graph to transform (default is the application graph).
         */
        void exportSRDAG(const std::string &path = "./srdag.dot", const PiSDF::Graph *graph = Spider::pisdfGraph());

        /**
         * @brief Export a PiSDF graph to a .dot file.
         * @param path   Path of the file.
         * @param graph  Graph to transform (default is the application graph).
         */
        void
        exportGraphToDOT(const std::string &path = "./graph.dot", const PiSDF::Graph *graph = Spider::pisdfGraph());

        /**
         * @brief Enable a given logger.
         * @param type @refitem LoggerType to enable.
         */
        template<Logger::Type type>
        void enableLogger();

        /**
         * @brief Disable a given logger.
         * @param type @refitem LoggerType to disable.
         */
        template<Spider::Logger::Type type>
        void disableLogger();
    }
}

#endif //SPIDER2_DEBUG_H
