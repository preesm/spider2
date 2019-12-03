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
#ifndef SPIDER2_API_REFINEMENT_H
#define SPIDER2_API_REFINEMENT_H

/* === Include(s) === */

#include <string>
#include <array>
#include <api/global.h>
#include <containers/containers.h>

namespace spider {

    /**
     * @brief Get the refinement register containing all refinement of the application.
     * @return Reference to the vector.
     */
    std::vector<pisdf::Refinement *> &refinementsRegister();

    namespace api {

        /* === Refinement API === */

        /**
         * @brief Reserve memory for the refinement register for faster insertion and lower memory footprint.
         * @param refinementCount
         */
        void precacheRefinementRegister(uint32_t refinementCount);

        /**
         * @brief Create a new Refinement.
         * @param name           Name of the refinement.
         * @param function       @refitem callback function associated to the Refinement.
         * @param paramINCount   Number of graph parameter read by the function.
         * @param paramOUTCount  Number of graph parameter set by the function.
         * @return pointer to newly created @refitem Refinement.
         */
        pisdf::Refinement *createRefinement(std::string name,
                                            callback function,
                                            uint32_t paramINCount = 0,
                                            uint32_t paramOUTCount = 0,
                                            StackID stack = StackID::PISDF);

        /**
         * @brief Register a refinement into Spider runtime.
         * @param refinement  Refinement to register.
         * @return Index in the refinement register of Spider.
         */
        uint32_t registerRefinement(pisdf::Refinement *refinement);

        /**
         * @brief Set the list of the graph parameter indexes used as input parameter.
         * @param refinement  Refinement to evaluate.
         * @param list        List of parameter indexes.
         */
        void setRefinementInputParams(pisdf::Refinement *refinement, std::initializer_list<uint32_t> list);

        /**
         * @brief Set the list of the graph parameter indexes used as output parameter.
         * @param refinement  Refinement to evaluate.
         * @param list        List of parameter indexes.
         */
        void setRefinementOutputParams(pisdf::Refinement *refinement, std::initializer_list<uint32_t> list);
    }
}

#endif //SPIDER2_API_REFINEMENT_H
