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
#ifndef SPIDER2_PE_H
#define SPIDER2_PE_H

/* === Include(s) === */

#include <string>
#include <common/Types.h>
#include <api/archi-api.h>

namespace spider {

    /* === Forward declaration(s) === */

    class Cluster;

    class MemoryUnit;

    /* === Class definition === */

    class PE {
    public:

        PE(uint32_t hwType,
           uint32_t hwIx,
           Cluster *cluster,
           std::string name = "unnamed-PE",
           PEType type = PEType::LRT,
           int32_t affinity = -1) : hwType_{ hwType },
                                    hwIx_{ hwIx },
                                    affinity_{ affinity },
                                    name_{ std::move(name) },
                                    type_{ type },
                                    cluster_{ cluster },
                                    attachedLRT_{ this } { };

        ~PE() = default;

        /* === Method(s) === */

        /* === Getter(s) === */

        /**
         * @brief Hardware type of the PE.
         * @return hardware type.
         */
        inline uint32_t hardwareType() const {
            return hwType_;
        }

        /**
         * @brief Hardware ix on which the PE runs.
         * @return hardward ix.
         */
        inline uint32_t hardwareIx() const {
            return hwIx_;
        }

        /**
         * @brief Get the name of the processing element.
         * @return name of the processing element, "unnamed-pe" if no name was provided.
         */
        inline std::string name() const {
            return name_;
        }

        inline int32_t affinity() const {
            return affinity_;
        }

        /**
         * @brief Get the unique ix of the PE in Spider.
         * @return ix of the PE in Spider.
         */
        inline size_t virtualIx() const {
            return virtIx_;
        }

        /**
         * @brief Fetch the LRT property of the PE.
         * @return true if the PE is an LRT, false else.
         */
        inline bool isLRT() const {
            return type_ == PEType::LRT;
        }

        /**
         * @brief Get the type of PE (processing, LRT)
         * @return @refitem Spider::PEType of the PE.
         */
        inline PEType spiderPEType() const {
            return type_;
        }

        /**
         * @brief Get the cluster associated to the processing element.
         * @return @refitem Cluster to which the PE belong.
         */
        inline Cluster *cluster() const {
            return cluster_;
        }

        /**
         * @brief Get the LRT that manages this PE.
         * @return pointer to managing LRT, nullptr if not set.
         */
        inline PE *attachedLRT() const {
            return attachedLRT_;
        }

        /**
         * @brief Get the state of the PE.
         * @return true if the PE is enabled, false else.
         */
        inline bool enabled() const {
            return status_;
        }


        /* === Setter(s) === */

        /**
         * @brief Enable the PE.
         */
        inline void enable() {
            status_ = true;
        }

        /**
         * @brief Disable the PE.
         */
        inline void disable() {
            status_ = false;
        }

        /**
         * @brief Set the name of the Processing Element.
         * @remark Calling this method will replace current name of the PE.
         * @param name  Name to set.
         */
        inline void setName(std::string name) {
            name_ = std::move(name);
        }

        /**
         * @brief The Spider::PEType of the Processing Element.
         * @remark Calling this method will replace current PEType of the PE.
         * @param type Type to set.
         */
        inline void setSpiderPEType(PEType type) {
            type_ = type;
        }

        /**
         * @brief Set the LRT attached to this PE.
         * @remark If lrt is nullptr, nothing happens.
         * @param lrt PE to set.
         */
        inline void setAttachedLRT(PE *lrt) {
            if (lrt) {
                attachedLRT_ = lrt;
            }
        }

        /**
         * @brief Sets the virtual ix of the PE.
         * @param ix Index to set.
         */
        inline void setVirtualIx(size_t ix) {
            virtIx_ = ix;
        }

    private:

        /* === Core properties === */

        uint32_t hwType_ = 0;             /* = S-LAM user hardware type = */
        uint32_t hwIx_ = 0;               /* = Hardware on which PE runs (core ix) = */
        int32_t affinity_ = -1;           /* = Thread affinity of the PE (optional) = */
        size_t virtIx_ = SIZE_MAX;        /* = Linear virtual unique IX used by Spider for fast access to PE = */
        std::string name_ = "unnamed-pe"; /* = S-LAM user name of the PE = */

        /* === Spider properties === */

        PEType type_ = PEType::LRT;      /* = PEType of the PE (see @refitem PEType) */
        Cluster *cluster_ = nullptr;     /* = Cluster to which the PE belong = */
        PE *attachedLRT_ = nullptr;      /* = Local Runtime PE attached to this PE = */
        bool status_ = true;             /* = Status of the PE (enabled = true, disabled = false) = */
    };
}

#endif //SPIDER2_PE_H
