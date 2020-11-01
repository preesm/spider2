/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2020)
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

/* === Include(s) === */

#include "spider2-reinforcement.h"

/* === Function(s) definition === */

spider::pisdf::Vertex *spider::rl::createTrain_actorSubgraph(std::string name,
                                                             spider::pisdf::Graph *parentGraph,
                                                             const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams) {
    /* == Create the subgraph == */
    auto *graph = spider::api::createSubgraph(parentGraph,
            /* = Name of the subgraph        = */ std::move(name),
            /* = Number of actors            = */ 25,
            /* = Number of edges             = */ 20,
            /* = Number of parameters        = */ 6,
            /* = Number of input interfaces  = */ 6,
            /* = Number of output interfaces = */ 2,
            /* = Number of config actors     = */ 1);

    /* === Creates the parameter(s) === */

    auto param_hidden_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "hidden_size",
            /* = Parent parameter       = */  parentGraphParams[0]);

    auto param_output_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "output_size",
            /* = Parent parameter       = */  parentGraphParams[1]);

    auto param_input_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "input_size",
            /* = Parent parameter       = */  parentGraphParams[2]);

    auto param_N = spider::api::createDynamicParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "N");

    auto param_weights_size = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "weights_size",
            /* = Expression of the parameter = */ "input_size * hidden_size + hidden_size * output_size");

    auto param_bias_size = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "bias_size",
            /* = Expression of the parameter = */ "hidden_size + output_size");

    /* === Set the input interface(s) === */

    auto *vertex_targets = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "targets");

    auto *vertex_inputs = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  1,
            /* = Name of the interface  = */ "inputs");

    auto *vertex_delta = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  2,
            /* = Name of the interface  = */ "delta");

    auto *vertex_learning_rate = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  3,
            /* = Name of the interface  = */ "learning_rate");

    auto *vertex_bias = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  4,
            /* = Name of the interface  = */ "bias");

    auto *vertex_weights = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  5,
            /* = Name of the interface  = */ "weights");

    /* === Set the output interface(s) === */

    auto *vertex_bias_out = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "bias_out");

    auto *vertex_weights_out = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  1,
            /* = Name of the interface  = */ "weights_out");

    /* === Creates the actor(s) == */

    auto *vertex_setNUpdate = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "setNUpdate",
            /* = Number of input edges  = */  2,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::CONFIG,
            /* = Kernel index (if any)  = */  kernels::SETNUMBEROFUPDATE);


    /* == Set the output parameters of the vertex setNUpdate == */
    spider::api::addOutputParamsToVertex(vertex_setNUpdate, { param_N });

    /* == Setting mappable constraints of the vertex setNUpdate == */
    spider::api::setVertexMappableOnPE(vertex_setNUpdate, PE_X86_CORE0, true);

    /* == Set the timings of the vertex setNUpdate == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_setNUpdate, TYPE_X86, "100");

    auto *vertex_sinkTargets = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "sinkTargets",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  0,
            /* = Type of the vertex     = */  pisdf::VertexType::END,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex sinkTargets == */
    spider::api::addInputParamsToVertex(vertex_sinkTargets, { param_output_size, param_N });

    /* == Setting mappable constraints of the vertex sinkTargets == */
    spider::api::setVertexMappableOnPE(vertex_sinkTargets, PE_X86_CORE0, true);

    /* == Set the timings of the vertex sinkTargets == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_sinkTargets, TYPE_X86, "100");

    auto *vertex_sinkInputs = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "sinkInputs",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  0,
            /* = Type of the vertex     = */  pisdf::VertexType::END,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex sinkInputs == */
    spider::api::addInputParamsToVertex(vertex_sinkInputs, { param_N, param_input_size });

    /* == Setting mappable constraints of the vertex sinkInputs == */
    spider::api::setVertexMappableOnPE(vertex_sinkInputs, PE_X86_CORE0, true);

    /* == Set the timings of the vertex sinkInputs == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_sinkInputs, TYPE_X86, "100");

    auto *vertex_SwitchLearning_Rate = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "SwitchLearning_Rate",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::FORK,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex SwitchLearning_Rate == */
    spider::api::addInputParamsToVertex(vertex_SwitchLearning_Rate, { param_N });

    /* == Setting mappable constraints of the vertex SwitchLearning_Rate == */
    spider::api::setVertexMappableOnPE(vertex_SwitchLearning_Rate, PE_X86_CORE0, true);

    /* == Set the timings of the vertex SwitchLearning_Rate == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_SwitchLearning_Rate, TYPE_X86, "100");

    auto *vertex_SwitchTargets = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "SwitchTargets",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::FORK,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex SwitchTargets == */
    spider::api::addInputParamsToVertex(vertex_SwitchTargets, { param_output_size, param_N });

    /* == Setting mappable constraints of the vertex SwitchTargets == */
    spider::api::setVertexMappableOnPE(vertex_SwitchTargets, PE_X86_CORE0, true);

    /* == Set the timings of the vertex SwitchTargets == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_SwitchTargets, TYPE_X86, "100");

    auto *vertex_SwitchInputs = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "SwitchInputs",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::FORK,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex SwitchInputs == */
    spider::api::addInputParamsToVertex(vertex_SwitchInputs, { param_N, param_input_size });

    /* == Setting mappable constraints of the vertex SwitchInputs == */
    spider::api::setVertexMappableOnPE(vertex_SwitchInputs, PE_X86_CORE0, true);

    /* == Set the timings of the vertex SwitchInputs == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_SwitchInputs, TYPE_X86, "100");

    auto *vertex_sinkLearning_Rate = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "sinkLearning_Rate",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  0,
            /* = Type of the vertex     = */  pisdf::VertexType::END,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex sinkLearning_Rate == */
    spider::api::addInputParamsToVertex(vertex_sinkLearning_Rate, { param_N });

    /* == Setting mappable constraints of the vertex sinkLearning_Rate == */
    spider::api::setVertexMappableOnPE(vertex_sinkLearning_Rate, PE_X86_CORE0, true);

    /* == Set the timings of the vertex sinkLearning_Rate == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_sinkLearning_Rate, TYPE_X86, "100");

    auto *vertex_Iterator = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "Iterator",
            /* = Number of input edges  = */  0,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::ACTORUPDATEITERATOR);


    /* == Set the input parameters used by rate expressions of the vertex Iterator == */
    spider::api::addInputParamsToVertex(vertex_Iterator, { param_N });

    /* == Setting mappable constraints of the vertex Iterator == */
    spider::api::setVertexMappableOnPE(vertex_Iterator, PE_X86_CORE0, true);

    /* == Set the timings of the vertex Iterator == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_Iterator, TYPE_X86, "100");

    /* === Creates the subgraph(s) === */

    auto *vertex_MPL_Update = spider::rl::createNetwork_train_iterSubgraph("MPL_Update", graph,
                                                                           { param_input_size, param_hidden_size,
                                                                             param_output_size, param_N });

    /* === Creates the edge(s) === */

    /* == Edge MPL_Update[weights_out] -> [weights]MPL_Update == */
    auto *edge_MPL_Update_weights_out__MPL_Update_weights = spider::api::createEdge(
            /* = Source vertex          = */  vertex_MPL_Update,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "((input_size * hidden_size + hidden_size * output_size) * (N > 0)) * 4",
            /* = Sink vertex            = */  vertex_MPL_Update,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "((input_size * hidden_size + hidden_size * output_size) * (N > 0)) * 4");

    /* == Set the delay on the edge == */
    spider::api::createLocalDelay(/* = Edge of the delay       = */  edge_MPL_Update_weights_out__MPL_Update_weights,
            /* = Expression of the delay = */ "(weights_size) * 4",
            /* = Setter of the delay     = */  vertex_weights,
            /* = Setter port ix          = */  0,
            /* = Setter rate expression  = */ "(weights_size) * 4",
            /* = Getter of the delay     = */  vertex_weights_out,
            /* = Getter port ix          = */  0,
            /* = Getter rate expression  = */ "(weights_size) * 4");

    /* == Edge MPL_Update[bias_out] -> [bias]MPL_Update == */
    auto *edge_MPL_Update_bias_out__MPL_Update_bias = spider::api::createEdge(
            /* = Source vertex          = */  vertex_MPL_Update,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "((hidden_size + output_size) * (N > 0)) * 4",
            /* = Sink vertex            = */  vertex_MPL_Update,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "((hidden_size + output_size) * (N > 0)) * 4");

    /* == Set the delay on the edge == */
    spider::api::createLocalDelay(/* = Edge of the delay       = */  edge_MPL_Update_bias_out__MPL_Update_bias,
            /* = Expression of the delay = */ "(bias_size) * 4",
            /* = Setter of the delay     = */  vertex_bias,
            /* = Setter port ix          = */  0,
            /* = Setter rate expression  = */ "(bias_size) * 4",
            /* = Getter of the delay     = */  vertex_bias_out,
            /* = Getter port ix          = */  0,
            /* = Getter rate expression  = */ "(bias_size) * 4");

    /* == Edge setNUpdate[updateVariance] -> [variance]setNUpdate == */
    auto *edge_setNUpdate_updateVariance__setNUpdate_variance = spider::api::createEdge(
            /* = Source vertex          = */  vertex_setNUpdate,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_setNUpdate,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_setNUpdate_updateVariance__setNUpdate_variance,
            /* = Expression of the delay = */ "(1) * 4");

    /* == Edge delta[delta] -> [delta]setNUpdate == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_delta,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_setNUpdate,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge Iterator[out] -> [iter_in]MPL_Update == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Iterator,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(N) * 4",
            /* = Sink vertex            = */  vertex_MPL_Update,
            /* = Sink port ix           = */  5,
            /* = sink rate expression   = */ "(N > 0) * 4");

    /* == Edge inputs[inputs] -> [in]SwitchInputs == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_inputs,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(input_size) * 4",
            /* = Sink vertex            = */  vertex_SwitchInputs,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(input_size) * 4");

    /* == Edge SwitchInputs[update] -> [inputs]MPL_Update == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_SwitchInputs,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "((N > 0) * input_size) * 4",
            /* = Sink vertex            = */  vertex_MPL_Update,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(input_size * (N > 0)) * 4");

    /* == Edge SwitchInputs[sink] -> [in]sinkInputs == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_SwitchInputs,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "((N == 0) * input_size) * 4",
            /* = Sink vertex            = */  vertex_sinkInputs,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "((N == 0) * input_size) * 4");

    /* == Edge targets[targets] -> [in]SwitchTargets == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_targets,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_SwitchTargets,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge SwitchTargets[update] -> [targets]MPL_Update == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_SwitchTargets,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "((N > 0) * output_size) * 4",
            /* = Sink vertex            = */  vertex_MPL_Update,
            /* = Sink port ix           = */  3,
            /* = sink rate expression   = */ "(output_size * (N > 0)) * 4");

    /* == Edge SwitchTargets[sink] -> [in]sinkTargets == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_SwitchTargets,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "((N == 0) * output_size) * 4",
            /* = Sink vertex            = */  vertex_sinkTargets,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "((N == 0) * output_size) * 4");

    /* == Edge learning_rate[learning_rate] -> [in]SwitchLearning_Rate == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_learning_rate,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_SwitchLearning_Rate,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge SwitchLearning_Rate[update] -> [learning_rate]MPL_Update == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_SwitchLearning_Rate,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "((N > 0)) * 4",
            /* = Sink vertex            = */  vertex_MPL_Update,
            /* = Sink port ix           = */  4,
            /* = sink rate expression   = */ "((N > 0)) * 4");

    /* == Edge SwitchLearning_Rate[sink] -> [in]sinkLearning_Rate == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_SwitchLearning_Rate,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "((N == 0) ) * 4",
            /* = Sink vertex            = */  vertex_sinkLearning_Rate,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "((N == 0)) * 4");

    /* == Return the graph as a Vertex == */
    return spider::api::convertGraphToVertex(graph);
}
