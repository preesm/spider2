/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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
#ifndef SPIDER2_PISDFGRAPH_H
#define SPIDER2_PISDFGRAPH_H

/* === Includes === */

#include <cstdint>
#include <common/containers/Set.h>
#include <graphs/pisdf/PiSDFVertex.h>
#include <graphs/pisdf/PiSDFEdge.h>
#include <graphs/pisdf/PiSDFParam.h>
#include <common/containers/LinkedList.h>

/* === Routine declaration(s) === */

using execRoutine = void (*)(void **, void **, std::uint64_t *, std::uint64_t *);

/* === Class definition === */

class PiSDFGraph {
public:

    /* === Methods === */

    /**
     * @brief Add a vertex to the graph.
     * @param vertex Vertex to add.
     */
    inline void addVertex(PiSDFVertex *vertex);

    /**
     * @brief Remove a vertex from the graph and update subgraph list if needed.
     * @remark If vertex is nullptr, nothing happens.
     * @param vertex Vertex to remove.
     * @throw @refitem SpiderException if vertex does not exist in the graph.
     */
    void removeVertex(PiSDFVertex *vertex);

    /**
     * @brief Add a subgraph to the graph.
     * @param vertex Hierarchical vertex from which the subgraph is added to the graph.
     * @throw @refitem SpiderException if vertex if not hierarchical.
     */
    void addSubGraph(PiSDFVertex *vertex);

    /**
     * @brief Add an edge to the graph.
     * @param edge Edge to add.
     */
    inline void addEdge(PiSDFEdge *edge);

    /**
     * @brief Remove an edge from the graph.
     * @remark If edge is nullptr, nothing happens.
     * @param edge Edge to remove.
     * @throw @refitem SpiderException if edge does not exist in the graph.
     */
    inline void removeEdge(PiSDFEdge *edge);

    /**
     * @brief Add an param to the graph.
     * @param param Param to add.
     */
    inline void addParam(PiSDFParam *param);

    /**
     * @brief Remove a param from the graph.
     * @remark If param is nullptr, nothing happens.
     * @param param Edge to remove.
     * @throw @refitem SpiderException if param does not exist in the graph.
     */
    inline void removeParam(PiSDFParam *param);

    /**
     * @brief Retrieve a parameter from its name.
     * @param name Name of the parameter.
     * @return pointer to the @refitem PiSDFParam if found, nullptr else.
     */
    inline PiSDFParam *findParam(const std::string &name) const;

    /* === Setters === */

    /**
     * @brief Set the parent vertex of the graph.
     * @param vertex Parent vertex.
     * @remark This method may call @refitem setSubGraph of class @refitem PiSDFVertex.
     * @throw @refitem SpiderException if already has a parent vertex or if nullptr.
     */
    inline void setParentVertex(PiSDFVertex *vertex);

    /* === Getters === */

    /**
     * @brief Get the number of vertices in the graph.
     * @remark This method exclude the number of interfaces and the number of config actors.
     * @return Total number of vertices.
     */
    inline std::uint64_t nVertices() const;

    /**
     * @brief Get the number of config actors in the graph.
     * @return Total number of config actors.
     */
    inline std::uint64_t nConfigs() const;

    /**
     * @brief Get the number of edges contained in the graph.
     * @return Number of edges.
     */
    inline std::uint64_t nEdges() const;

    /**
     * @brief Get the number of sub-graphs.
     * @return Number of sub-graphs.
     */
    inline std::uint64_t nSubGraphs() const;

    /**
     * @brief Get the total number of interfaces.
     * @return Total number of interfaces.
     */
    inline std::uint64_t nInterfaces() const;

    /**
     * @brief Get the number of input interfaces.
     * @return Number of input interfaces.
     */
    inline std::uint64_t nInputInterfaces() const;

    /**
     * @brief Get the number of output interfaces.
     * @return Number of output interfaces.
     */
    inline std::uint64_t nOutputInterfaces() const;

    /**
     * @brief Get the parent vertex of current sub-graph.
     * @return Parent vertex, nullptr if top graph.
     */
    inline PiSDFVertex *parentVertex() const;

    /**
    * @brief A const reference on the set of vertices. Useful for iterating on the vertices.
    * @return const reference to vertex set
    */
    inline const Spider::Set<PiSDFVertex *> vertices() const;

    /**
    * @brief A const reference on the set of output interfaces. Useful for iterating on the input interfaces.
    * @return const reference to input interface set
    */
    inline const Spider::Set<PiSDFVertex *> inputInterfaces() const;

    /**
    * @brief A const reference on the set of output interfaces. Useful for iterating on the output interfaces.
    * @return const reference to output interface set
    */
    inline const Spider::Set<PiSDFVertex *> outputInterfaces() const;

    /**
    * @brief A const reference on the set of edges. Useful for iterating on the edges.
    * @return const reference to edge set
    */
    inline const Spider::Set<PiSDFEdge *> edges() const;

    /**
    * @brief A const reference on the set of params. Useful for iterating on the params.
    * @return const reference to param set
    */
    inline const Spider::Set<PiSDFParam *> params() const;

    /**
     * @brief A const reference to the LinkedList of subgraph. Useful for iterating on the subgraphs.
     * @return const reference to subgraph LinkedList.
     */
    inline const Spider::LinkedList<PiSDFGraph *> subgraphs() const;

    /**
     * @brief Get the static property of the graph.
     * @return true if graph is static, false else.
     */
    inline bool isStatic() const;

    /**
     * @brief  Check if the graph contains dynamic parameter(s).
     * @return true if graph contains at least one dynamic parameter, false else.
     */
    inline bool containsDynamicParameters() const;

private:
    PiSDFVertex *parentVertex_ = nullptr;

    Spider::Set<PiSDFVertex *> vertexSet_;
    Spider::Set<PiSDFVertex *> configSet_;
    Spider::LinkedList<PiSDFGraph *> subgraphList_;
    Spider::Set<PiSDFVertex *> inputInterfaceSet_;
    Spider::Set<PiSDFVertex *> outputInterfaceSet_;
    Spider::Set<PiSDFEdge *> edgeSet_;
    Spider::Set<PiSDFParam *> paramSet_;

    bool static_ = true;
    bool hasDynamicParameters_ = false;
};

/* === Inline methods === */

void PiSDFGraph::addVertex(PiSDFVertex *vertex) {
    switch (vertex->type()) {
        case PiSDFType::VERTEX:
            if (vertex->subType() == PiSDFSubType::GRAPH) {
                addSubGraph(vertex);
            } else {
                vertexSet_.add(vertex);
            }
            break;
        case PiSDFType::CONFIG_VERTEX:
            configSet_.add(vertex);
            break;
        case PiSDFType::INTERFACE_VERTEX:
            if (vertex->subType() == PiSDFSubType::INPUT) {
                inputInterfaceSet_.add(vertex);
            } else {
                outputInterfaceSet_.add(vertex);
            }
            break;
        default:
            throwSpiderException("Unsupported type of vertex.");
    }
}

void PiSDFGraph::addEdge(PiSDFEdge *edge) {
    edgeSet_.add(edge);
}

void PiSDFGraph::removeEdge(PiSDFEdge *edge) {
    if (!edge) {
        return;
    }
    if (!edgeSet_.contains(edge)) {
        throwSpiderException("Trying to remove an edge not from this graph.");
    }
    edgeSet_.remove(edge);
    Spider::destroy(edge);
    Spider::deallocate(edge);
}

void PiSDFGraph::addParam(PiSDFParam *param) {
    paramSet_.add(param);
    if (param->isDynamic() && static_) {
        static_ = false;

        /* == We need to propagate this property up in the hierarchy == */
        auto *parent = parentVertex_;
        while (parent) {
            auto *graph = parent->containingGraph();

            /* == If graph was already dynamic then information is already propagated == */
            if (!graph->static_) {
                break;
            }
            graph->static_ = false;
            parent = graph->parentVertex_;
        }
    }
}

void PiSDFGraph::removeParam(PiSDFParam *param) {
    if (!param) {
        return;
    }
    if (!paramSet_.contains(param)) {
        throwSpiderException("Trying to remove an edge not from this graph.");
    }
    paramSet_.remove(param);
    Spider::destroy(param);
    Spider::deallocate(param);
}

PiSDFParam *PiSDFGraph::findParam(const std::string &name) const {
    for (auto &p : paramSet_) {
        if (p->name() == name) {
            return p;
        }
    }
    return nullptr;
}

void PiSDFGraph::setParentVertex(PiSDFVertex *vertex) {
    if (parentVertex_) {
        throwSpiderException("Graph already has a parent vertex.");
    }
    if (!vertex) {
        throwSpiderException("Trying to set nullptr parent vertex.");
    }
    parentVertex_ = vertex;
    if (vertex->subgraph() != this) {
        vertex->setSubGraph(this);
    }
}

std::uint64_t PiSDFGraph::nVertices() const {
    return vertexSet_.size();
}

std::uint64_t PiSDFGraph::nConfigs() const {
    return configSet_.size();
}

std::uint64_t PiSDFGraph::nEdges() const {
    return edgeSet_.size();
}

std::uint64_t PiSDFGraph::nSubGraphs() const {
    return subgraphList_.size();
}

std::uint64_t PiSDFGraph::nInterfaces() const {
    return inputInterfaceSet_.size() + outputInterfaceSet_.size();
}

std::uint64_t PiSDFGraph::nInputInterfaces() const {
    return inputInterfaceSet_.size();
}

std::uint64_t PiSDFGraph::nOutputInterfaces() const {
    return outputInterfaceSet_.size();
}

PiSDFVertex *PiSDFGraph::parentVertex() const {
    return parentVertex_;
}

const Spider::Set<PiSDFVertex *> PiSDFGraph::vertices() const {
    return vertexSet_;
}

const Spider::Set<PiSDFVertex *> PiSDFGraph::inputInterfaces() const {
    return inputInterfaceSet_;
}

const Spider::Set<PiSDFVertex *> PiSDFGraph::outputInterfaces() const {
    return outputInterfaceSet_;
}

const Spider::Set<PiSDFEdge *> PiSDFGraph::edges() const {
    return edgeSet_;
}

const Spider::Set<PiSDFParam *> PiSDFGraph::params() const {
    return paramSet_;
}

const Spider::LinkedList<PiSDFGraph *> PiSDFGraph::subgraphs() const {
    return subgraphList_;
}

bool PiSDFGraph::isStatic() const {
    return static_;
}

bool PiSDFGraph::containsDynamicParameters() const {
    return hasDynamicParameters_;
}


#endif //SPIDER2_PISDFGRAPH_H
