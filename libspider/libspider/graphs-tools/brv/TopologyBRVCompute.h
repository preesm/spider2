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
#ifndef SPIDER2_TOPOLOGYBRVCOMPUTE_H
#define SPIDER2_TOPOLOGYBRVCOMPUTE_H

/* === Include(s) === */

#include <graphs-tools/brv/BRVCompute.h>

/* === Class definition === */

class TopologyBRVCompute final : public BRVCompute {
public:

    explicit TopologyBRVCompute(PiSDFGraph *graph) : BRVCompute(graph) { };

    TopologyBRVCompute(PiSDFGraph *graph, const spider::vector<PiSDFParam *> &params) : BRVCompute(graph, params) { };

    ~TopologyBRVCompute() override = default;

    /* === Method(s) === */

    void execute() override;

    /* === Getter(s) === */

    /* === Setter(s) === */

private:

    /* === Private method(s) === */

    /**
     * @brief Check if a vertex is executable by checking its rate.
     * @param vertex  Vertex to test.
     * @return true if at least one rate is not null, false else.
     */
    bool isVertexExecutable(const PiSDFAbstractVertex *vertex) const;

    /**
     * @brief Check if an edge is valid.
     * An edge is valid if is connected to two executable VERTEX and has non null rates.
     * @param edge           Edge to test.
     * @param vertexIxArray  Indexes in the topology matrix of the vertices of the graph.
     * @return true if edge is valid, false else.
     */
    static bool isEdgeValid(const PiSDFEdge *edge, spider::Array<std::int32_t> &vertexIxArray);

    /**
     * @brief Compute the nullspace of the topology matrix using the pivot method.
     * Repetition vector of the connected component is then deduced from the nullspace.
     * @param topologyMatrix Topology matrix of the connected component.
     * @param nMatVertices   Number of vertices (=number of columns) in the topology matrix.
     * @param nMatEdges      Number of edges (=number of rows) in the topology matrix.
     * @param vertexIxArray  Indexes in the topology matrix of the vertices of the graph.
     * @param component      Current connected component.
     *
     * @throw @refitem SpiderException if nullspace can not be computed.
     */
    static void computeBRVFromNullSpace(spider::Array<std::int64_t> &topologyMatrix,
                                        std::uint32_t nMatVertices,
                                        std::uint32_t nMatEdges,
                                        spider::Array<std::int32_t> &vertexIxArray,
                                        const BRVComponent &component);
};

/* === Inline method(s) === */



#endif //SPIDER2_TOPOLOGYBRVCOMPUTE_H
