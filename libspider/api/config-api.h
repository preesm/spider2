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
#ifndef SPIDER2_CONFIG_API_H
#define SPIDER2_CONFIG_API_H

/* === Methods prototype === */

namespace spider {

    namespace api {

        /* === Methods for setting flags value in Spider === */

        /**
         * @brief Enable the traces in Spider.
         */
        void enableExportTrace();

        /**
         * @brief Disable the traces in Spider (default behavior).
         */
        void disableExportTrace();

        /**
         * @brief Enable the export of the SRDAG in Spider.
         * @remark default path is "./srdag.dot"
         */
        void enableExportSRDAG();

        /**
         * @brief Disable the export of the SRDAG in Spider (default behavior).
         */
        void disableExportSRDAG();

        /**
         * @brief Enable the Verbose mode in Spider.
         */
        void enableVerbose();

        /**
         * @brief Disable the Verbose mode in Spider (default behavior).
         */
        void disableVerbose();

        /**
         * @brief Enable the SRDAG optimizations (default behavior).
         */
        void enableSRDAGOptims();

        /**
         * @brief Disable the SRDAG optimizations.
         */
        void disableSRDAGOptims();

        /* === Getters for static variables === */

        /**
         * @brief Get the trace flag value.
         * @return value of trace flag.
         */
        bool exportTraceEnabled();

        /**
         * @brief Get the export srdag flag value.
         * @return value of export srdag flag.
         */
        bool exportSRDAGEnabled();

        /**
         * @brief Get the verbose flag value.
         * @return value of verbose flag.
         */
        bool verboseEnabled();

        /**
         * @brief Get the srdagOptim flag value.
         * @return value of srdagOptim flag.
         */
        bool shouldOptimizeSRDAG();
    }
}

#endif //SPIDER2_CONFIG_API_H
