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
#ifndef SPIDER2_PISDFGRAPH_H
#define SPIDER2_PISDFGRAPH_H

/* === Includes === */

#include <cstdint>
#include <containers/Set.h>
#include <graphs/pisdf/PiSDFVertex.h>
#include <graphs/pisdf/PiSDFInterface.h>
#include <graphs/pisdf/PiSDFEdge.h>
#include <graphs/pisdf/PiSDFParam.h>
#include <containers/LinkedList.h>

/* === Routine declaration(s) === */

using execRoutine = void (*)(void **, void **, std::uint64_t *, std::uint64_t *);

/* === Class definition === */

class PiSDFGraph {
public:

    PiSDFGraph(std::string name,
               std::uint64_t nActors,
               std::uint64_t nEdges,
               std::uint64_t nParams = 0,
               std::uint64_t nInputInterfaces = 0,
               std::uint64_t nOutputInterfaces = 0,
               std::uint64_t nConfigActors = 0);

    PiSDFGraph(PiSDFVertex *parent,
               std::string name,
               std::uint64_t nActors,
               std::uint64_t nEdges,
               std::uint64_t nParams = 0,
               std::uint64_t nInputInterfaces = 0,
               std::uint64_t nOutputInterfaces = 0,
               std::uint64_t nConfigActors = 0);

    ~PiSDFGraph();

    /* === Methods === */

    /**
     * @brief Add a vertex to the graph.
     * @param vertex Vertex to add.
     */
    inline void addVertex(PiSDFVertex *vertex);

    /**
     * @brief Add an interface to the graph.
     * @param interface Interface to add.
     */
    inline void addInterface(PiSDFInterface *interface);

    /**
     * @brief Remove a vertex from the graph and update subgraph list if needed.
     * @remark If vertex is nullptr, nothing happens.
     * @param vertex Vertex to remove.
     * @throw @refitem SpiderException if vertex does not exist in the graph.
     */
    void removeVertex(PiSDFVertex *vertex);

    /**
     * @brief Remove a subgraph from the graph.
     * @remark If subgraph is nullptr, nothing happens.
     * @param subgraph Subgraph to remove.
     * @throw @refitem SpiderException if subgraph does not exist in the graph.
     */
    void removeSubgraph(PiSDFGraph *subgraph);

    /**
     * @brief Add a subgraph to the graph.
     * @param subgraph Subgraph to be added to the graph.
     */
    void addSubgraph(PiSDFGraph *subgraph);

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
    void removeEdge(PiSDFEdge *edge);

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


    /**
     * @brief Export the graph to a dot file.
     * @param path Path of the file.
     */
    void exportDot(const std::string &path = "./pisdf.dot") const;

    /**
     * @brief Export the graph to a dot file.
     * @param file    File pointer to write to.
     * @param offset  Tab offset in the file.
     */
    void exportDot(FILE *file, const std::string &offset) const;

    /**
     * @brief Pre-cache space for storing vertices. Accelerate insertion after.
     * If (capacity - size) < n, the pre-cache space is added to current container size.
     * If (capacity - size) >= n, nothing happens, container already possess sufficient space.
     * @param n Number of vertices to pre-cache.
     */
    inline void precacheVertices(std::uint64_t n);

    /**
     * @brief Pre-cache space for storing config vertices. Accelerate insertion after.
     * If (capacity - size) < n, the pre-cache space is added to current container size.
     * If (capacity - size) >= n, nothing happens, container already possess sufficient space.
     * @param n Number of config vertices to pre-cache.
     */
    inline void precacheConfigVertices(std::uint64_t n);

    /**
     * @brief Pre-cache space for storing edges. Accelerate insertion after.
     * If (capacity - size) < n, the pre-cache space is added to current container size.
     * If (capacity - size) >= n, nothing happens, container already possess sufficient space.
     * @param n Number of edges to pre-cache.
     */
    inline void precacheEdges(std::uint64_t n);

    /**
     * @brief Pre-cache space for storing params. Accelerate insertion after.
     * If (capacity - size) < n, the pre-cache space is added to current container size.
     * If (capacity - size) >= n, nothing happens, container already possess sufficient space.
     * @param n Number of params to pre-cache.
     */
    inline void precacheParams(std::uint64_t n);

    /* === Setters === */


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
     * @brief Get the number of params contained in the graph.
     * @return Number of params.
     */
    inline std::uint64_t nParams() const;

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
    inline PiSDFVertex *parent() const;

    /**
    * @brief A const reference on the set of vertices. Useful for iterating on the vertices.
    * @return const reference to vertex set
    */
    inline const Spider::vector<PiSDFVertex *> &vertices() const;

    /**
    * @brief A const reference on the set of vertices. Useful for iterating on the vertices.
    * @return const reference to vertex set
    */
    inline const Spider::vector<PiSDFVertex *> &configActors() const;

    /**
    * @brief A const reference on the set of output interfaces. Useful for iterating on the input interfaces.
    * @return const reference to input interface set
    */
    inline const Spider::vector<PiSDFInterface *> &inputInterfaces() const;

    /**
    * @brief A const reference on the set of output interfaces. Useful for iterating on the output interfaces.
    * @return const reference to output interface set
    */
    inline const Spider::vector<PiSDFInterface *> &outputInterfaces() const;

    /**
    * @brief A const reference on the set of edges. Useful for iterating on the edges.
    * @return const reference to edge set
    */
    inline const Spider::vector<PiSDFEdge *> &edges() const;

    /**
    * @brief A const reference on the set of params. Useful for iterating on the params.
    * @return const reference to param set
    */
    inline const Spider::vector<PiSDFParam *> &params() const;

    /**
     * @brief A const reference to the LinkedList of subgraph. Useful for iterating on the subgraphs.
     * @return const reference to subgraph LinkedList.
     */
    inline const Spider::LinkedList<PiSDFGraph *> &subgraphs() const;

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

    /**
     * @brief Get the string of the name of the graph.
     * @return string of the name.
     */
    inline const std::string &name() const;

private:
    std::string name_ = "topgraph";
    Spider::vector<PiSDFVertex *> vertexVector_;
    Spider::vector<PiSDFEdge *> edgeVector_;
    Spider::vector<PiSDFParam *> paramVector_;
    Spider::vector<PiSDFInterface *> inputInterfaceVector_;
    Spider::vector<PiSDFInterface *> outputInterfaceVector_;
    Spider::vector<PiSDFVertex *> configVertexVector_;
    Spider::LinkedList<PiSDFGraph *> subgraphVector_;

    bool static_ = true;
    bool hasDynamicParameters_ = false;
    PiSDFVertex *parent_ = nullptr;

    /**
     * @brief Export to DOT from a given FILE pointer.
     * @param file     File to which to export.
     * @param offset   TAB offset in the file.
     */
    void exportDotHelper(FILE *file, const std::string &offset) const;
};

/* === Inline methods === */

void PiSDFGraph::addVertex(PiSDFVertex *vertex) {
    switch (vertex->type()) {
        case PiSDFVertexType::DUPLICATE:
        case PiSDFVertexType::ROUNDBUFFER:
        case PiSDFVertexType::FORK:
        case PiSDFVertexType::JOIN:
        case PiSDFVertexType::INIT:
        case PiSDFVertexType::END:
        case PiSDFVertexType::DELAY:
        case PiSDFVertexType::NORMAL:
            vertex->setIx(vertexVector_.size());
            vertexVector_.push_back(vertex);
            break;
        case PiSDFVertexType::HIERARCHICAL:
            vertex->setIx(vertexVector_.size());
            vertexVector_.push_back(vertex);
            break;
        case PiSDFVertexType::CONFIG:
            vertex->setIx(vertexVector_.size());
            configVertexVector_.push_back(vertex);
            break;
        case PiSDFVertexType::INTERFACE:
            break;
        default:
            throwSpiderException("Unsupported type of vertex.");
    }
}

void PiSDFGraph::addInterface(PiSDFInterface *interface) {
    switch (interface->interfaceType()) {
        case PiSDFInterfaceType::INPUT:
            interface->setIx(inputInterfaceVector_.size());
            inputInterfaceVector_.push_back(interface);
            break;
        case PiSDFInterfaceType::OUTPUT:
            interface->setIx(outputInterfaceVector_.size());
            outputInterfaceVector_.push_back(interface);
            break;
        default:
            throwSpiderException("Unsupported type of interface");
    }
}

void PiSDFGraph::addEdge(PiSDFEdge *edge) {
    edge->setIx(edgeVector_.size());
    edgeVector_.push_back(edge);
}

void PiSDFGraph::addParam(PiSDFParam *param) {
    /* == Check if a parameter with the same name already exists in the scope of this graph == */
    if (findParam(param->name())) {
        throwSpiderException("Parameter [%s] already exist in graph [%s].", param->name().c_str(), name_.c_str());
    }
    param->setIx(paramVector_.size());
    paramVector_.push_back(param);
    if (param->isDynamic() && static_) {
        static_ = false;

        /* == We need to propagate this property up in the hierarchy == */
        auto *parent = parent_->containingGraph();
        while (parent) {
            /* == If graph was already dynamic then information is already propagated == */
            if (!parent->static_) {
                break;
            }
            parent->static_ = false;
            parent = parent->parent_->containingGraph();
        }
    }
}

PiSDFParam *PiSDFGraph::findParam(const std::string &name) const {
    for (auto &p : paramVector_) {
        if (p->name() == name) {
            return p;
        }
    }
    return nullptr;
}

void PiSDFGraph::precacheVertices(std::uint64_t n) {
    auto currentCache = vertexVector_.capacity() - vertexVector_.size();
    if (currentCache < n) {
        vertexVector_.reserve(vertexVector_.capacity() + (n - currentCache));
    }
}

void PiSDFGraph::precacheConfigVertices(std::uint64_t n) {
    auto currentCache = configVertexVector_.capacity() - configVertexVector_.size();
    if (currentCache < n) {
        configVertexVector_.reserve(configVertexVector_.capacity() + (n - currentCache));
    }
}

void PiSDFGraph::precacheEdges(std::uint64_t n) {
    auto currentCache = edgeVector_.capacity() - edgeVector_.size();
    if (currentCache < n) {
        edgeVector_.reserve(edgeVector_.capacity() + (n - currentCache));
    }
}

void PiSDFGraph::precacheParams(std::uint64_t n) {
    auto currentCache = paramVector_.capacity() - paramVector_.size();
    if (currentCache < n) {
        paramVector_.reserve(paramVector_.capacity() + (n - currentCache));
    }
}

std::uint64_t PiSDFGraph::nVertices() const {
    return vertexVector_.size();
}

std::uint64_t PiSDFGraph::nConfigs() const {
    return configVertexVector_.size();
}

std::uint64_t PiSDFGraph::nEdges() const {
    return edgeVector_.size();
}

std::uint64_t PiSDFGraph::nParams() const {
    return paramVector_.size();
}

std::uint64_t PiSDFGraph::nSubGraphs() const {
    return subgraphVector_.size();
}

std::uint64_t PiSDFGraph::nInterfaces() const {
    return nInputInterfaces() + nOutputInterfaces();
}

std::uint64_t PiSDFGraph::nInputInterfaces() const {
    return inputInterfaceVector_.size();
}

std::uint64_t PiSDFGraph::nOutputInterfaces() const {
    return outputInterfaceVector_.size();
}

PiSDFVertex *PiSDFGraph::parent() const {
    return parent_;
}

const Spider::vector<PiSDFVertex *> &PiSDFGraph::vertices() const {
    return vertexVector_;
}

const Spider::vector<PiSDFVertex *> &PiSDFGraph::configActors() const {
    return configVertexVector_;
}

const Spider::vector<PiSDFInterface *> &PiSDFGraph::inputInterfaces() const {
    return inputInterfaceVector_;
}

const Spider::vector<PiSDFInterface *> &PiSDFGraph::outputInterfaces() const {
    return outputInterfaceVector_;
}

const Spider::vector<PiSDFEdge *> &PiSDFGraph::edges() const {
    return edgeVector_;
}

const Spider::vector<PiSDFParam *> &PiSDFGraph::params() const {
    return paramVector_;
}

const Spider::LinkedList<PiSDFGraph *> &PiSDFGraph::subgraphs() const {
    return subgraphVector_;
}

bool PiSDFGraph::isStatic() const {
    return static_;
}

bool PiSDFGraph::containsDynamicParameters() const {
    return hasDynamicParameters_;
}

const std::string &PiSDFGraph::name() const {
    return name_;
}

#endif //SPIDER2_PISDFGRAPH_H
