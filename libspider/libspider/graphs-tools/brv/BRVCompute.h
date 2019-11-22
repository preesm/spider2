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
#ifndef SPIDER2_BRVCOMPUTE_H
#define SPIDER2_BRVCOMPUTE_H

/* === Include(s) === */

#include <containers/containers.h>
#include <containers/array.h>
#include <graphs/pisdf/common/Types.h>

/* === Struct definition === */

struct BRVComponent {
    uint32_t nEdges = 0;
    spider::vector<PiSDFAbstractVertex *> vertices;

    BRVComponent() = default;
};

/* === Enumeration(s) === */

/**
 * @brief BRV compute methods enumeration.
 */
enum class BRVMethod : uint8_t {
    LCM_BASED,      /*! The LCM based method of computing the BRV (default) */
    TOPOLOGY_BASED, /*! The Topology matrix based method of computing the BRV (legacy) */
};

/* === Class definition === */

class BRVCompute {
public:

    explicit BRVCompute(const PiSDFGraph *graph);

    BRVCompute(const PiSDFGraph *graph, const spider::vector<PiSDFParam *> &params);

    virtual ~BRVCompute() = default;

    /* === Method(s) === */

    /**
     * @brief Compute the repetition vector values of the graph.
     */
    virtual void execute() = 0;

    /* === Getter(s) === */

    /* === Setter(s) === */

protected:
    const PiSDFGraph *graph_ = nullptr;
    const spider::vector<PiSDFParam *> &params_;
    spider::vector<BRVComponent> connectedComponents_;

    /* === Protected method(s) === */

    /**
     * @brief Print the BRV (only if VERBOSE is enabled).
     */
    void print() const;

    /* === Static method(s) === */

    /**
     * @brief Extract all connected components of the current graph.
     * @param component Reference to the connected component to be filled.
     * @param keyArray  Key array for fast information on which vertex have already been visited.
     */
    static void
    extractConnectedComponent(BRVComponent &component, spider::array<const PiSDFAbstractVertex *> &keyArray);

    /**
     * @brief Extract the edges of a given connected component.
     * @param component  Connected component.
     * @return Array of Edge of the connected component.
     */
    static spider::array<const PiSDFEdge *> extractEdges(const BRVComponent &component);

    /**
     * @brief Update the repetition vector values depending on interfaces and config actors rates.
     * @param edgeArray Edge array of the current connected components.
     */
    void updateBRV(const BRVComponent &component);
};

/* === Inline method(s) === */



#endif //SPIDER2_BRVCOMPUTE_H
