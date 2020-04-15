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
#ifndef SPIDER2_INTERMEMORYBUS_H
#define SPIDER2_INTERMEMORYBUS_H

/* === Include(s) === */

namespace spider {

    /* === Forward declaration === */

    class MemoryBus;

    class Cluster;

    /* === Class definition === */

    class InterMemoryBus {
    public:
        /**
         * @brief Builds an inter cluster memory bus.
         * @param clusterA   Pointer to cluster A.
         * @param clusterB   Pointer to cluster B.
         * @param busAToB    Pointer to the @refitem MemoryBus from A to B (can be nullptr).
         * @param busBToA    Pointer to the @refitem MemoryBus from B to A (can be nullptr).
         * @throws spider::Exception if either of clusterA or clusterB is nullptr.
         */
        InterMemoryBus(Cluster *clusterA, Cluster *clusterB, MemoryBus *busAToB, MemoryBus *busBToA);

        ~InterMemoryBus();

        /* === Method(s) === */

        /**
         * @brief Returns the @refitem MemoryBus of the direction A -> B.
         * @param clusterA  Pointer to the first cluster.
         * @param clusterB  Pointer to the second cluster.
         * @return pointer to appropriate @refitem MemoryBus, nullptr if A or B is not related to this InterMemoryBus.
         * @throws spider::Exception if either of clusterA or clusterB is nullptr.
         */
        MemoryBus *get(Cluster *clusterA, Cluster *clusterB);

    private:
        Cluster *clusterA_ = nullptr;
        Cluster *clusterB_ = nullptr;
        MemoryBus *busAToB_ = nullptr;
        MemoryBus *busBToA_ = nullptr;
    };
}
#endif //SPIDER2_INTERMEMORYBUS_H
