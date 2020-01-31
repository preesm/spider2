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

/* === Includes === */

#include <api/pisdf-api.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/InHeritedParam.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/NonExecVertex.h>

/* === Methods implementation === */

spider::pisdf::Graph *&spider::pisdf::applicationGraph() {
    static pisdf::Graph *graph = nullptr;
    return graph;
}

spider::pisdf::Graph *spider::api::createUserApplicationGraph(std::string name,
                                                              size_t actorCount,
                                                              size_t edgeCount,
                                                              size_t paramCount,
                                                              size_t inIFCount,
                                                              size_t outIFCount,
                                                              size_t cfgActorCount) {
    if (pisdf::applicationGraph()) {
        throwSpiderException("Can have only one user application graph inside spider.");
    }
    pisdf::applicationGraph() = createGraph(std::move(name),
                                            actorCount,
                                            edgeCount,
                                            paramCount,
                                            inIFCount,
                                            outIFCount,
                                            cfgActorCount);
    return pisdf::applicationGraph();
}

spider::pisdf::Graph *spider::api::createGraph(std::string name,
                                               size_t actorCount,
                                               size_t edgeCount,
                                               size_t paramCount,
                                               size_t inIFCount,
                                               size_t outIFCount,
                                               size_t cfgActorCount) {
    if (name == "app-graph") {
        throwSpiderException("Unauthorized name: \"app-graph\" is a reserved name for graphs by Spider.");
    }
    return make<pisdf::Graph>(StackID::PISDF,
                              std::move(name),
                              actorCount,
                              edgeCount,
                              paramCount,
                              inIFCount,
                              outIFCount,
                              cfgActorCount);
}

spider::pisdf::Graph *spider::api::createSubgraph(pisdf::Graph *graph,
                                                  std::string name,
                                                  size_t actorCount,
                                                  size_t edgeCount,
                                                  size_t paramCount,
                                                  size_t inIFCount,
                                                  size_t outIFCount,
                                                  size_t cfgActorCount) {
    if (!graph) {
        throwSpiderException("trying to create a subgraph %s with no parent.", name.c_str());
    }
    if (name == "app-graph") {
        throwSpiderException("Unauthorized name: \"app-graph\" is a reserved name for graphs by Spider.");
    }
    auto *subgraph = make<pisdf::Graph>(StackID::PISDF,
                                        std::move(name),
                                        actorCount,
                                        edgeCount,
                                        paramCount,
                                        inIFCount,
                                        outIFCount,
                                        cfgActorCount);
    graph->addVertex(subgraph);
    return subgraph;
}

spider::pisdf::Vertex *spider::api::convertGraphToVertex(pisdf::Graph *graph) {
    return graph;
}

spider::pisdf::Vertex *spider::api::createVertexFromType(pisdf::Graph *graph,
                                                         std::string name,
                                                         size_t inputEdgeCount,
                                                         size_t outputEdgeCount,
                                                         pisdf::VertexType type,
                                                         size_t kernelIx) {
    switch (type) {
        case spider::pisdf::VertexType::NORMAL: {
            auto *vertex = spider::api::createVertex(graph, std::move(name), inputEdgeCount, outputEdgeCount);
            /* == Special actors kernels are added internally == */
            vertex->runtimeInformation()->setKernelIx(pisdf::SPECIAL_KERNEL_COUNT + kernelIx);
            return vertex;
        }
        case spider::pisdf::VertexType::CONFIG:
            return spider::api::createConfigActor(graph, std::move(name), inputEdgeCount, outputEdgeCount);
        case spider::pisdf::VertexType::FORK:
            return spider::api::createFork(graph, std::move(name), outputEdgeCount);
        case spider::pisdf::VertexType::JOIN:
            return spider::api::createJoin(graph, std::move(name), inputEdgeCount);
        case spider::pisdf::VertexType::REPEAT:
            return spider::api::createRepeat(graph, std::move(name));
        case spider::pisdf::VertexType::DUPLICATE:
            return spider::api::createDuplicate(graph, std::move(name), outputEdgeCount);
        case spider::pisdf::VertexType::TAIL:
            return spider::api::createTail(graph, std::move(name), inputEdgeCount);
        case spider::pisdf::VertexType::HEAD:
            return spider::api::createHead(graph, std::move(name), inputEdgeCount);
        case spider::pisdf::VertexType::INIT:
            return spider::api::createInit(graph, std::move(name));
        case spider::pisdf::VertexType::END:
            return spider::api::createEnd(graph, std::move(name));
        default:
            throwSpiderException("vertex type not found");
    }
}

spider::pisdf::Vertex *spider::api::createVertex(pisdf::Graph *graph,
                                                 std::string name,
                                                 size_t edgeINCount,
                                                 size_t edgeOUTCount) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *vertex = make<pisdf::ExecVertex>(StackID::PISDF, std::move(name), edgeINCount, edgeOUTCount);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::Vertex *spider::api::createNonExecVertex(pisdf::Graph *graph,
                                                        std::string name,
                                                        size_t edgeINCount,
                                                        size_t edgeOUTCount) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *vertex = make<pisdf::NonExecVertex>(StackID::PISDF, std::move(name), edgeINCount, edgeOUTCount);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::Vertex *spider::api::createFork(pisdf::Graph *graph, std::string name, size_t edgeOUTCount) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *vertex = make<pisdf::ForkVertex>(StackID::PISDF, std::move(name), edgeOUTCount);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(0);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::Vertex *spider::api::createJoin(pisdf::Graph *graph, std::string name, size_t edgeINCount) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *vertex = make<pisdf::JoinVertex>(StackID::PISDF, std::move(name), edgeINCount);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(1);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::Vertex *spider::api::createHead(pisdf::Graph *graph, std::string name, size_t edgeINCount) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *vertex = make<pisdf::HeadVertex>(StackID::PISDF, std::move(name), edgeINCount);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(2);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::Vertex *spider::api::createTail(pisdf::Graph *graph, std::string name, size_t edgeINCount) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *vertex = make<pisdf::TailVertex>(StackID::PISDF, std::move(name), edgeINCount);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(3);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::Vertex *spider::api::createDuplicate(pisdf::Graph *graph, std::string name, size_t edgeOUTCount) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *vertex = make<pisdf::DuplicateVertex>(StackID::PISDF, std::move(name), edgeOUTCount);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(5);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::Vertex *spider::api::createRepeat(pisdf::Graph *graph, std::string name) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *vertex = make<pisdf::RepeatVertex>(StackID::PISDF, std::move(name));
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(4);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::Vertex *spider::api::createInit(pisdf::Graph *graph, std::string name) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *vertex = make<pisdf::InitVertex>(StackID::PISDF, std::move(name));
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(6);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::Vertex *spider::api::createEnd(pisdf::Graph *graph, std::string name) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *vertex = make<pisdf::EndVertex>(StackID::PISDF, std::move(name));
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(7);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::Vertex *spider::api::createConfigActor(pisdf::Graph *graph,
                                                      std::string name,
                                                      size_t edgeINCount,
                                                      size_t edgeOUTCount) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *vertex = make<pisdf::ConfigVertex>(StackID::PISDF, std::move(name), edgeINCount, edgeOUTCount);
    graph->addVertex(vertex);
    return vertex;
}

spider::pisdf::Vertex *spider::api::setInputInterfaceName(pisdf::Graph *graph, size_t ix, std::string name) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *interface = graph->inputInterface(ix);
    if (!interface) {
        throwSpiderException("no input interface at index %"
                                     PRIu32
                                     " in graph [%s]", graph->name().c_str());
    }
    interface->setName(std::move(name));
    return interface;
}

spider::pisdf::Vertex *spider::api::setOutputInterfaceName(pisdf::Graph *graph, size_t ix, std::string name) {
    if (!graph) {
        throwSpiderException("nullptr for graph.");
    }
    auto *interface = graph->outputInterface(ix);
    if (!interface) {
        throwSpiderException("no output interface at index %"
                                     PRIu32
                                     " in graph [%s]", graph->name().c_str());
    }
    interface->setName(std::move(name));
    return interface;
}

/* === Param creation API === */

std::shared_ptr<spider::pisdf::Param>
spider::api::createStaticParam(pisdf::Graph *graph, std::string name, int64_t value) {
    auto param = make_shared<pisdf::Param>(StackID::PISDF, std::move(name), value);
    if (graph) {
        graph->addParam(param);
    }
    return param;
}

std::shared_ptr<spider::pisdf::Param>
spider::api::createStaticParam(pisdf::Graph *graph, std::string name, std::string expression) {
    if (graph) {
        auto param = make_shared<pisdf::Param>(StackID::PISDF, std::move(name),
                                               Expression(std::move(expression), graph->params()));
        graph->addParam(param);
        return param;
    }
    return make_shared<pisdf::Param>(StackID::PISDF, std::move(name), Expression(std::move(expression)));
}

std::shared_ptr<spider::pisdf::Param>
spider::api::createDynamicParam(pisdf::Graph *graph, std::string name) {
    if (graph) {
        auto param = make_shared<pisdf::DynamicParam>(StackID::PISDF, std::move(name), Expression(0));
        graph->addParam(param);
        return param;
    }
    return make_shared<pisdf::DynamicParam>(StackID::PISDF, std::move(name), Expression(0));
}

std::shared_ptr<spider::pisdf::Param>
spider::api::createDynamicParam(pisdf::Graph *graph, std::string name, std::string expression) {
    if (graph) {
        auto param = make_shared<pisdf::DynamicParam>(StackID::PISDF, std::move(name),
                                                      Expression(std::move(expression), graph->params()));
        graph->addParam(param);
        return param;
    }
    return make_shared<pisdf::DynamicParam>(StackID::PISDF, std::move(name), Expression(std::move(expression)));
}

std::shared_ptr<spider::pisdf::Param>
spider::api::createInheritedParam(pisdf::Graph *graph, std::string name, pisdf::Param *parent) {
    if (!parent) {
        throwSpiderException("Cannot instantiate inherited parameter [%s] with null parent.", name.c_str());
    }
    if (!parent->dynamic()) {
        return createStaticParam(graph, std::move(name), parent->value());
    }
    if (graph) {
        auto param = make_shared<pisdf::InHeritedParam>(StackID::PISDF, std::move(name), parent);
        graph->addParam(param);
        return param;
    }
    return make_shared<pisdf::InHeritedParam>(StackID::PISDF, std::move(name), parent);
}

std::shared_ptr<spider::pisdf::Param>
spider::api::createInheritedParam(pisdf::Graph *graph, std::string name, const std::string &parentName) {
    if (!graph) {
        throwSpiderException("Cannot instantiate inherited parameter from name in a nullptr graph.");
    }
    if (!graph->graph()) {
        throwSpiderException("Cannot instantiate inherited parameter from name if graph [%s] has no parent graph.",
                             graph->name().c_str());
    }
    auto *parent = graph->graph()->paramFromName(parentName);
    if (!parent) {
        throwSpiderException("Cannot instantiate inherited parameter [%s] with null parent.", name.c_str());
    }
    if (!parent->dynamic()) {
        return createStaticParam(graph, std::move(name), parent->value());
    }
    auto param = make_shared<pisdf::InHeritedParam>(StackID::PISDF, std::move(name), parent);
    graph->addParam(param);
    return param;
}

void spider::api::addInputParamToVertex(pisdf::Vertex *vertex, std::shared_ptr<spider::pisdf::Param> param) {
    if (!param || !vertex) {
        return;
    }
    if (param->graph() != vertex->graph()) {
        throwSpiderException("parameter [%s] and vertex [%s] are not in the same graph.",
                             param->name().c_str(),
                             vertex->name().c_str());
    }
    vertex->addInputParameter(std::move(param));
}

void spider::api::addInputRefinementParamToVertex(pisdf::Vertex *vertex, std::shared_ptr<spider::pisdf::Param> param) {
    if (!param || !vertex) {
        return;
    }
    if (param->graph() != vertex->graph()) {
        throwSpiderException("parameter [%s] and vertex [%s] are not in the same graph.",
                             param->name().c_str(),
                             vertex->name().c_str());
    }
    vertex->addRefinementParameter(param);
    vertex->addInputParameter(std::move(param));
}

void spider::api::addOutputParamToVertex(pisdf::Vertex *vertex, std::shared_ptr<spider::pisdf::Param> param) {
    if (!param || !vertex) {
        return;
    }
    if (vertex->subtype() != pisdf::VertexType::CONFIG) {
        throwSpiderException("Failed to set parameter [%s] as output param of vertex [%s]: not a config actor.",
                             param->name().c_str(),
                             vertex->name().c_str());
    }
    if (param->graph() != vertex->graph()) {
        throwSpiderException("parameter [%s] and vertex [%s] are not in the same graph.",
                             param->name().c_str(),
                             vertex->name().c_str());
    }
    vertex->addOutputParameter(std::move(param));
}


/* === Edge API === */

spider::pisdf::Edge *spider::api::createEdge(pisdf::Vertex *source,
                                             size_t srcPortIx,
                                             std::string srcRateExpression,
                                             pisdf::Vertex *sink,
                                             size_t snkPortIx,
                                             std::string snkRateExpression) {
    try {
        auto *edge = make<pisdf::Edge>(StackID::PISDF,
                                       source,
                                       srcPortIx,
                                       Expression(std::move(srcRateExpression), source->graph()->params()),
                                       sink,
                                       snkPortIx,
                                       Expression(std::move(snkRateExpression), sink->graph()->params()));

        source->graph()->addEdge(edge);
        return edge;
    } catch (spider::Exception &e) {
        throw e;
    }
}

spider::pisdf::Edge *spider::api::createEdge(pisdf::Vertex *source,
                                             size_t srcPortIx,
                                             int64_t srcRate,
                                             pisdf::Vertex *sink,
                                             size_t snkPortIx,
                                             int64_t snkRate) {
    try {
        auto *edge = make<pisdf::Edge>(StackID::PISDF,
                                       source,
                                       srcPortIx,
                                       Expression(srcRate),
                                       sink,
                                       snkPortIx,
                                       Expression(snkRate));
        source->graph()->addEdge(edge);
        return edge;
    } catch (spider::Exception &e) {
        throw e;
    }

}

static spider::Expression checkAndGetExpression(spider::pisdf::Edge *edge, std::string delayExpression) {
    if (!edge) {
        throwSpiderException("Can not create Edge on nullptr edge.");
    }
    auto *graph = edge->graph();
    auto expression = spider::Expression(std::move(delayExpression), graph->params());
    if (expression.dynamic()) {
        throwSpiderException("Spider 2.0 does not yet support dynamic delay.");
    }
    return expression;
}

spider::pisdf::Delay *spider::api::createDelay(pisdf::Edge *edge,
                                               std::string delayExpression,
                                               pisdf::ExecVertex *setter,
                                               size_t setterPortIx,
                                               std::string setterRateExpression,
                                               pisdf::ExecVertex *getter,
                                               size_t getterPortIx,
                                               std::string getterRateExpression,
                                               bool persistent) {
    auto expression = checkAndGetExpression(edge, std::move(delayExpression));
    auto setterExpr = setter ? std::move(setterRateExpression) : std::to_string(expression.value());
    auto getterExpr = getter ? std::move(getterRateExpression) : std::to_string(expression.value());
    return make<pisdf::Delay>(StackID::PISDF,
                              std::move(expression),
                              edge,
                              setter,
                              setterPortIx,
                              Expression(std::move(setterExpr), edge->graph()->params()),
                              getter,
                              getterPortIx,
                              Expression(std::move(getterExpr), edge->graph()->params()),
                              persistent);
}

spider::pisdf::Delay *spider::api::createDelay(pisdf::Edge *edge,
                                               int64_t value,
                                               pisdf::ExecVertex *setter,
                                               size_t setterPortIx,
                                               int64_t setterRate,
                                               pisdf::ExecVertex *getter,
                                               size_t getterPortIx,
                                               int64_t getterRate,
                                               bool persistent) {
    return make<pisdf::Delay>(StackID::PISDF,
                              Expression(value),
                              edge,
                              setter,
                              setterPortIx,
                              Expression(setter ? setterRate : value),
                              getter,
                              getterPortIx,
                              Expression(getter ? getterRate : value),
                              persistent);
}

spider::pisdf::Delay *spider::api::createPersistentDelay(spider::pisdf::Edge *edge,
                                                         std::string delayExpression) {
    if (delayExpression == "0" && spider::log::enabled()) {
        spider::log::warning("delay with null value on edge [%s] ignored.\n",
                             edge->name().c_str());
        return nullptr;
    }
    auto expression = checkAndGetExpression(edge, std::move(delayExpression));
    auto *graph = edge->graph();
    const auto value = expression.value();
    while (!graph->isTopGraph()) {
        /* == We need to make it persist up to the top-level == */
        /* == 0. Creates the interfaces == */
        auto *input = make<pisdf::InputInterface, StackID::PISDF>("in_" + edge->name());
        auto *output = make<pisdf::OutputInterface, StackID::PISDF>("out_" + edge->name());
        graph->addInputInterface(input);
        graph->addOutputInterface(output);

        /* == 1. Connect the delay to the edge and the interfaces == */
        make<pisdf::Delay, StackID::PISDF>(Expression(value), edge,
                                           input, 0, Expression(value),
                                           output, 0, Expression(value),
                                           false);

        /* == 2. Creates the edge around the graph == */
        edge = make<pisdf::Edge, StackID::PISDF>(graph, graph->outputEdgeCount() - 1, Expression(value),
                                                 graph, graph->inputEdgeCount() - 1, Expression(value));

        /* == 3. Update the current graph == */
        graph = graph->graph();
        graph->addEdge(edge);
    }
    return make<pisdf::Delay, StackID::PISDF>(std::move(expression),
                                              edge,
                                              nullptr,
                                              0,
                                              Expression(value),
                                              nullptr,
                                              0,
                                              Expression(value),
                                              true);
}

spider::pisdf::Delay *spider::api::createLocalPersistentDelay(pisdf::Edge *edge,
                                                              std::string delayExpression,
                                                              int_fast32_t levelCount) {
    if (delayExpression == "0" && spider::log::enabled()) {
        spider::log::warning("delay with null value on edge [%s] ignored.\n",
                             edge->name().c_str());
        return nullptr;
    } else if (levelCount < 0) {
        return createPersistentDelay(edge, std::move(delayExpression));
    }
    auto expression = checkAndGetExpression(edge, std::move(delayExpression));
    int_fast32_t currentLevelCount = 0;
    auto *graph = edge->graph();
    const auto value = expression.value();
    while ((currentLevelCount < levelCount) && !graph->isTopGraph()) {
        /* == 0. Creates the interfaces == */
        auto *input = make<pisdf::InputInterface, StackID::PISDF>("in_" + edge->name());
        auto *output = make<pisdf::OutputInterface, StackID::PISDF>("out_" + edge->name());
        graph->addInputInterface(input);
        graph->addOutputInterface(output);

        /* == 1. Connect the delay to the edge and the interfaces == */
        make<pisdf::Delay, StackID::PISDF>(Expression(value), edge,
                                           input, 0, Expression(value),
                                           output, 0, Expression(value),
                                           false);

        /* == 2. Creates the edge around the graph == */
        edge = make<pisdf::Edge, StackID::PISDF>(graph, graph->outputEdgeCount() - 1, Expression(value),
                                                 graph, graph->inputEdgeCount() - 1, Expression(value));

        /* == 3. Update the current graph == */
        graph = graph->graph();
        graph->addEdge(edge);

        currentLevelCount++;
    }
    bool persistent = graph->isTopGraph();
    return make<pisdf::Delay, StackID::PISDF>(std::move(expression), edge,
                                              nullptr, 0, Expression(value),
                                              nullptr, 0, Expression(value),
                                              persistent);
}

spider::pisdf::Delay *spider::api::createLocalDelay(pisdf::Edge *edge,
                                                    std::string delayExpression,
                                                    pisdf::ExecVertex *setter,
                                                    size_t setterPortIx,
                                                    std::string setterRateExpression,
                                                    pisdf::ExecVertex *getter,
                                                    size_t getterPortIx,
                                                    std::string getterRateExpression) {
    if (delayExpression == "0" && spider::log::enabled()) {
        spider::log::warning("delay with null value on edge [%s] ignored.\n",
                             edge->name().c_str());
        return nullptr;
    }
    auto expression = checkAndGetExpression(edge, std::move(delayExpression));
    auto setterExpr = setter ? std::move(setterRateExpression) : std::to_string(expression.value());
    auto getterExpr = getter ? std::move(getterRateExpression) : std::to_string(expression.value());
    return make<pisdf::Delay>(StackID::PISDF,
                              std::move(expression),
                              edge,
                              setter,
                              setterPortIx,
                              Expression(std::move(setterExpr), edge->graph()->params()),
                              getter,
                              getterPortIx,
                              Expression(std::move(getterExpr), edge->graph()->params()),
                              false);
}
