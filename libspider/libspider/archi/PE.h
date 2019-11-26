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
#ifndef SPIDER2_PE_H
#define SPIDER2_PE_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <spider-api/archi.h>
#include <ostream>

namespace spider {

    /* === Forward declaration(s) === */

    class Cluster;

    class MemoryUnit;

    /* === Class definition === */

    class PE {
    public:

        PE(uint32_t hwType,
           uint32_t hwIx,
           uint32_t virtIx,
           Cluster *cluster,
           std::string name = "unnamed-PE",
           spider::PEType spiderPEType = spider::PEType::LRT_PE,
           spider::HWType spiderHWType = spider::HWType::PHYS_PE);

        ~PE() = default;

        /* === Method(s) === */

        /* === Getter(s) === */

        /**
         * @brief Hardware type of the PE.
         * @return hardware type.
         */
        inline uint32_t hardwareType() const;

        /**
         * @brief Hardware ix on which the PE runs.
         * @return hardward ix.
         */
        inline uint32_t hardwareIx() const;

        /**
         * @brief Ix set by the user in the architecture description of the platform.
         * @return virtual ix.
         */
        inline uint32_t virtualIx() const;

        /**
         * @brief Get the name of the processing element.
         * @return name of the processing element, "unnamed-pe" if no name was provided.
         */
        inline std::string name() const;

        /**
         * @brief Get the cluster associated to the processing element.
         * @return @refitem Cluster to which the PE belong.
         */
        inline Cluster *cluster() const;

        /**
         * @brief Get the type of PE (processing, LRT)
         * @return @refitem Spider::PEType of the PE.
         */
        inline spider::PEType spiderPEType() const;

        /**
         * @brief Get the hw type of the PE (virtual or physical)
         * @return @refitem Spider::HWType of the PE.
         */
        inline spider::HWType spiderHardwareType() const;

        /**
         * @brief Get the ix of the PE inside its cluster.
         * @return ix of the PE inside the cluster.
         */
        inline uint32_t clusterPEIx() const;

        /**
         * @brief Get the unique ix of the PE in Spider.
         * @return ix of the PE in Spider.
         */
        inline uint32_t spiderPEIx() const;

        /**
         * @brief Get the state of the PE.
         * @return true if the PE is enabled, false else.
         */
        inline bool enabled() const;

        /**
         * @brief Fetch the LRT property of the PE.
         * @return true if the PE is an LRT, false else.
         */
        inline bool isLRT() const;

        /**
         * @brief Get the memory unit attached to the cluster to which the PE belong.
         * @return reference to the @refitem MemoryUnit of the PE.
         */
        MemoryUnit &memoryUnit() const;

        /**
         * @brief Get the LRT that manages this PE.
         * @return pointer to managing LRT, nullptr if not set.
         */
        inline PE *managingLRT() const;

        /**
         * @brief IX of the LRT handling this PE.
         * @return IX value.
         */
        inline uint32_t managingLRTIx() const;

        /* === Setter(s) === */

        /**
         * @brief Enable the PE.
         * @remark update the number of enabled PE in the associated Cluster.
         */
        void enable();

        /**
         * @brief Disable the PE.
         * @remark update the number of enabled PE in the associated Cluster.
         */
        void disable();

        /**
         * @brief Set the Processing Element ix inside the associated cluster.
         * @remark This method override current value. It is up to the user to ensure consistency in the index.
         * @param ix  Ix to set.
         */
        inline void setClusterPEIx(uint32_t ix);

        /**
         * @brief Set the name of the Processing Element.
         * @remark Calling this method will replace current name of the PE.
         * @param name  Name to set.
         */
        inline void setName(const std::string &name);

        /**
         * @brief The Spider::PEType of the Processing Element.
         * @remark Calling this method will replace current PEType of the PE.
         * @param type Type to set.
         */
        inline void setSpiderPEType(spider::PEType type);

        /**
         * @brief The Spider::HWType of the Processing Element.
         * @remark Calling this method will replace current HWType of the PE.
         * @param type Type to set.
         */
        inline void setSpiderHWType(spider::HWType type);

        inline void setManagingLRT(PE *managingLRT);

    private:

        /* === Core properties === */

        uint32_t hwType_ = 0;        /* = S-LAM user hardware type = */
        uint32_t hwIx_ = 0;          /* = Hardware on which PE runs (core ix) = */
        uint32_t virtIx_ = 0;        /* = S-LAM user ix = */
        std::string name_ = "unnamed-pe"; /* = S-LAM user name of the PE = */

        /* === Spider properties === */

        Cluster *cluster_ = nullptr;    /* = Cluster to which the PE belong = */
        uint32_t clusterPEIx_ = 0; /* = Ix inside the cluster (used internally by spider) = */
        uint32_t spiderPEIx_ = 0;  /* = Unique Ix of the PE inside spider (used internally by spider) = */
        spider::PEType spiderPEType_ = spider::PEType::LRT_PE;
        spider::HWType spiderHWType_ = spider::HWType::PHYS_PE;
        PE *managingLRT_ = nullptr; /* == LRT handling this PE (self if PE is an LRT) == */
        uint32_t managingLRTIx_ = UINT32_MAX; /* == Ix of the LRT handling this PE == */
        bool enabled_ = false;

        /* === Private method(s) === */
    };

    /* === Inline method(s) === */

    uint32_t PE::hardwareType() const {
        return hwType_;
    }

    uint32_t PE::hardwareIx() const {
        return hwIx_;
    }

    uint32_t PE::virtualIx() const {
        return virtIx_;
    }

    std::string PE::name() const {
        return name_;
    }

    Cluster *PE::cluster() const {
        return cluster_;
    }

    spider::PEType PE::spiderPEType() const {
        return spiderPEType_;
    }

    spider::HWType PE::spiderHardwareType() const {
        return spiderHWType_;
    }

    uint32_t PE::clusterPEIx() const {
        return clusterPEIx_;
    }

    uint32_t PE::spiderPEIx() const {
        return spiderPEIx_;
    }

    bool PE::enabled() const {
        return enabled_;
    }

    bool PE::isLRT() const {
        return spiderPEType_ != spider::PEType::PE_ONLY;
    }

    PE *PE::managingLRT() const {
        return managingLRT_;
    }

    uint32_t PE::managingLRTIx() const {
        return managingLRTIx_;
    }

    void PE::setClusterPEIx(uint32_t ix) {
        clusterPEIx_ = ix;
    }

    void PE::setName(const std::string &name) {
        name_ = name;
    }

    void PE::setSpiderPEType(spider::PEType type) {
        spiderPEType_ = type;
    }

    void PE::setSpiderHWType(spider::HWType type) {
        spiderHWType_ = type;
    }

    void PE::setManagingLRT(PE *managingLRT) {
        managingLRT_ = managingLRT;
        managingLRTIx_ = managingLRT->managingLRTIx_;
    }
}

#endif //SPIDER2_PE_H
