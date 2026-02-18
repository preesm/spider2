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
#ifndef SPIDER2_CLUSTER_H
#define SPIDER2_CLUSTER_H

/* === Include(s) === */

#include <algorithm>
#include <api/archi-api.h>
#include <archi/PE.h>
#include <common/Types.h>
#include <containers/array.h>
#include <containers/array_view.h>

namespace spider {

    /* === Class definition === */

    class Cluster {
    public:

        Cluster(size_t PECount, MemoryInterface *memoryInterface);

        ~Cluster();

        Cluster(Cluster &&) = default;

        Cluster(const Cluster &) = delete;

        Cluster &operator=(Cluster &&) = default;

        Cluster &operator=(const Cluster &) = delete;

        /* === Method(s) === */

        /**
         * @brief Add a processing element to the cluster.
         * @param PE Processing element to add.
         * @throws @refitem std::out_of_range if cluster is already full.
         */
        void addPE(PE *pe);

        /**
         * @brief Set the state (enabled or disabled) of a processing element in the cluster.
         * @param ix      Ix of the processing element.
         * @param status  Status of the PE to set (true = enabled, false = disabled).
         * @throws std::out_of_range if PE ix is out of bound.
         */
        inline void setPEStatus(size_t ix, bool status) {
            auto handle = make_view(PEArray_, PECount_);
            status ? handle.at(ix)->enable() : handle.at(ix)->disable();
        }

        /* === Getter(s) === */

        /**
         * @brief Get the array of processing element of the cluster.
         * @return const reference to the @refitem spider::array of @refitem PE of the cluster.
         */
        inline array_view<PE *> peArray() const {
            return make_view(PEArray_, PECount_);
        }

        /**
         * @brief Get the memory interface of the cluster.
         * @return pointer to the @refitem MemoryInterface of the cluster.
         */
        inline MemoryInterface *memoryInterface() const {
            return memoryInterface_;
        }

        /**
         * @brief Get a given processing element from the cluster.
         * @param ix  Ix of the processing element in the cluster.
         * @return const reference of the @refitem ProcessingElement
         * @throws @refitem std::out_of_range if ix is out of bound
         */
        inline PE *at(size_t ix) const {
            return make_view(PEArray_, PECount_).at(ix);
        }

        /**
         * @brief Get the number of processing element actually inside the cluster.
         * @return number of @refitem PE inside the cluster.
         */
        inline size_t PECount() const {
            return PECount_;
        }

        /**
         * @brief Get the number of local runtime in the cluster.
         * @return number of local runtime inside the cluster.
         */
        inline size_t LRTCount() const {
            return LRTCount_;
        }

        /**
         * @brief Get the PE type of the cluster.
         * @remark This method return the value of @refitem PE::hardwareType() method of the first PE.
         * @return PE type of the cluster.
         */
        inline uint32_t PEType() const {
            return PEArray_[0]->hardwareType();
        }

        /**
         * @brief  Get the cluster ix (unique among clusters).
         * @return Ix of the cluster.
         */
        inline size_t ix() const {
            return ix_;
        }

        /**
         * @brief Get the platform of the cluster.
         * @return @refitem Platform of the cluster.
         */
        static inline Platform *platform() {
            return archi::platform();
        }

        /* === Setter(s) === */

        /**
         * @brief Set the cluster ix inside the Platform.
         * @param ix Ix to set.
         */
        inline void setIx(size_t ix) {
            ix_ = ix;
        }

    private:

        /* === Core properties === */

        PE **PEArray_ = nullptr;                     /* = Array of PE contained in the Cluster = */
        MemoryInterface *memoryInterface_ = nullptr; /* = Pointer to the MemoryInterface for intra Cluster communications = */

        /* === Spider properties === */

        size_t LRTCount_ = 0; /* = Number of Local Runtime inside this Cluster = */
        size_t PECount_ = 0;  /* = Number of currently added PE in the Cluster */
        size_t ix_ = 0;       /* = Linear index of the Cluster in the Platform */

    };
}
#endif //SPIDER2_CLUSTER_H
