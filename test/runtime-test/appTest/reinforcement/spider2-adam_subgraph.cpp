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

spider::pisdf::Vertex *spider::rl::createAdamSubgraph(std::string name,
                                                      spider::pisdf::Graph *parentGraph,
                                                      const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams) {
    /* == Create the subgraph == */
    auto *graph = spider::api::createSubgraph(parentGraph,
            /* = Name of the subgraph        = */ std::move(name),
            /* = Number of actors            = */ 22,
            /* = Number of edges             = */ 28,
            /* = Number of parameters        = */ 3,
            /* = Number of input interfaces  = */ 8,
            /* = Number of output interfaces = */ 4,
            /* = Number of config actors     = */ 0);

    /* === Creates the parameter(s) === */

    auto param_layer_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "layer_size",
            /* = Parent parameter       = */  parentGraphParams[0]);

    auto param_input_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "input_size",
            /* = Parent parameter       = */  parentGraphParams[1]);

    auto param_weights_size = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "weights_size",
            /* = Expression of the parameter = */ "input_size * layer_size");

    /* === Set the input interface(s) === */

    auto *vertex_gradients = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "gradients");

    auto *vertex_weights = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  1,
            /* = Name of the interface  = */ "weights");

    auto *vertex_bias = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  2,
            /* = Name of the interface  = */ "bias");

    auto *vertex_first_order_moments = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  3,
            /* = Name of the interface  = */ "first_order_moments");

    auto *vertex_second_order_moments = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  4,
            /* = Name of the interface  = */ "second_order_moments");

    auto *vertex_betas = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  5,
            /* = Name of the interface  = */ "betas");

    auto *vertex_epsilon = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  6,
            /* = Name of the interface  = */ "epsilon");

    auto *vertex_learning_rate = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  7,
            /* = Name of the interface  = */ "learning_rate");

    /* === Set the output interface(s) === */

    auto *vertex_second_order_moments_out = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "second_order_moments_out");

    auto *vertex_first_order_moments_out = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  1,
            /* = Name of the interface  = */ "first_order_moments_out");

    auto *vertex_weights_out = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  2,
            /* = Name of the interface  = */ "weights_out");

    auto *vertex_bias_out = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  3,
            /* = Name of the interface  = */ "bias_out");

    /* === Creates the actor(s) == */

    auto *vertex_BroadcastEpsilon = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastEpsilon",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastEpsilon == */
    spider::api::addInputParamsToVertex(vertex_BroadcastEpsilon, { param_weights_size });

    /* == Setting mappable constraints of the vertex BroadcastEpsilon == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastEpsilon, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastEpsilon == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastEpsilon, TYPE_X86, "100");

    auto *vertex_ForkFirstOrder = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "ForkFirstOrder",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::FORK,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex ForkFirstOrder == */
    spider::api::addInputParamsToVertex(vertex_ForkFirstOrder, { param_layer_size, param_weights_size });

    /* == Setting mappable constraints of the vertex ForkFirstOrder == */
    spider::api::setVertexMappableOnPE(vertex_ForkFirstOrder, PE_X86_CORE0, true);

    /* == Set the timings of the vertex ForkFirstOrder == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_ForkFirstOrder, TYPE_X86, "100");

    auto *vertex_optimizeBias = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "optimizeBias",
            /* = Number of input edges  = */  7,
            /* = Number of output edges = */  3,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::APPLYADAMOPTIMIZER);


    /* == Set the input parameters needed by the refinement of the vertex optimizeBias == */
    spider::api::addInputRefinementParamToVertex(vertex_optimizeBias, param_layer_size);

    /* == Setting mappable constraints of the vertex optimizeBias == */
    spider::api::setVertexMappableOnPE(vertex_optimizeBias, PE_X86_CORE0, true);

    /* == Set the timings of the vertex optimizeBias == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_optimizeBias, TYPE_X86, "100");

    auto *vertex_BroadcastBetas = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastBetas",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastBetas == */
    spider::api::addInputParamsToVertex(vertex_BroadcastBetas, { param_weights_size });

    /* == Setting mappable constraints of the vertex BroadcastBetas == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastBetas, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastBetas == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastBetas, TYPE_X86, "100");

    auto *vertex_ForkGradients = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "ForkGradients",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::FORK,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex ForkGradients == */
    spider::api::addInputParamsToVertex(vertex_ForkGradients, { param_layer_size, param_weights_size });

    /* == Setting mappable constraints of the vertex ForkGradients == */
    spider::api::setVertexMappableOnPE(vertex_ForkGradients, PE_X86_CORE0, true);

    /* == Set the timings of the vertex ForkGradients == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_ForkGradients, TYPE_X86, "100");

    auto *vertex_ForkSecondOrder = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "ForkSecondOrder",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::FORK,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex ForkSecondOrder == */
    spider::api::addInputParamsToVertex(vertex_ForkSecondOrder, { param_layer_size, param_weights_size });

    /* == Setting mappable constraints of the vertex ForkSecondOrder == */
    spider::api::setVertexMappableOnPE(vertex_ForkSecondOrder, PE_X86_CORE0, true);

    /* == Set the timings of the vertex ForkSecondOrder == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_ForkSecondOrder, TYPE_X86, "100");

    auto *vertex_JoinFirstOrder = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "JoinFirstOrder",
            /* = Number of input edges  = */  2,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::JOIN,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex JoinFirstOrder == */
    spider::api::addInputParamsToVertex(vertex_JoinFirstOrder, { param_layer_size, param_weights_size });

    /* == Setting mappable constraints of the vertex JoinFirstOrder == */
    spider::api::setVertexMappableOnPE(vertex_JoinFirstOrder, PE_X86_CORE0, true);

    /* == Set the timings of the vertex JoinFirstOrder == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_JoinFirstOrder, TYPE_X86, "100");

    auto *vertex_JoinSecondOrder = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "JoinSecondOrder",
            /* = Number of input edges  = */  2,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::JOIN,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex JoinSecondOrder == */
    spider::api::addInputParamsToVertex(vertex_JoinSecondOrder, { param_layer_size, param_weights_size });

    /* == Setting mappable constraints of the vertex JoinSecondOrder == */
    spider::api::setVertexMappableOnPE(vertex_JoinSecondOrder, PE_X86_CORE0, true);

    /* == Set the timings of the vertex JoinSecondOrder == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_JoinSecondOrder, TYPE_X86, "100");

    auto *vertex_optimizeWeights = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "optimizeWeights",
            /* = Number of input edges  = */  7,
            /* = Number of output edges = */  3,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::APPLYADAMOPTIMIZER);


    /* == Set the input parameters needed by the refinement of the vertex optimizeWeights == */
    spider::api::addInputRefinementParamToVertex(vertex_optimizeWeights, param_weights_size);

    /* == Setting mappable constraints of the vertex optimizeWeights == */
    spider::api::setVertexMappableOnPE(vertex_optimizeWeights, PE_X86_CORE0, true);

    /* == Set the timings of the vertex optimizeWeights == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_optimizeWeights, TYPE_X86, "100");

    auto *vertex_BroadcastLearningRate = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastLearningRate",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastLearningRate == */
    spider::api::addInputParamsToVertex(vertex_BroadcastLearningRate, { param_weights_size });

    /* == Setting mappable constraints of the vertex BroadcastLearningRate == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastLearningRate, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastLearningRate == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastLearningRate, TYPE_X86, "100");

    /* === Creates the subgraph(s) === */

    /* === Creates the edge(s) === */

    /* == Edge optimizeWeights[fo_moment_out] -> [weights_out]weights_out == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_optimizeWeights,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(weights_size) * 4",
            /* = Sink vertex            = */  vertex_weights_out,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(weights_size) * 4");

    /* == Edge optimizeBias[fo_moment_out] -> [bias_out]bias_out == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_optimizeBias,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(layer_size) * 4",
            /* = Sink vertex            = */  vertex_bias_out,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(layer_size) * 4");

    /* == Edge weights[weights] -> [gradients]optimizeWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_weights,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(weights_size) * 4",
            /* = Sink vertex            = */  vertex_optimizeWeights,
            /* = Sink port ix           = */  3,
            /* = sink rate expression   = */ "(weights_size) * 4");

    /* == Edge BroadcastLearningRate[out_0] -> [betas]optimizeWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastLearningRate,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(1 * (weights_size > 0)) * 4",
            /* = Sink vertex            = */  vertex_optimizeWeights,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1 * (weights_size > 0)) * 4");

    /* == Edge BroadcastLearningRate[out_1] -> [betas]optimizeBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastLearningRate,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1 * (weights_size > 0)) * 4",
            /* = Sink vertex            = */  vertex_optimizeBias,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1 * (layer_size > 0)) * 4");

    /* == Edge learning_rate[learning_rate] -> [in]BroadcastLearningRate == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_learning_rate,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_BroadcastLearningRate,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1 * (weights_size > 0)) * 4");

    /* == Edge epsilon[epsilon] -> [in]BroadcastEpsilon == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_epsilon,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 8",
            /* = Sink vertex            = */  vertex_BroadcastEpsilon,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1 * (weights_size > 0)) * 8");

    /* == Edge BroadcastEpsilon[out_0] -> [epsilon]optimizeBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastEpsilon,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1 * (weights_size > 0)) * 8",
            /* = Sink vertex            = */  vertex_optimizeBias,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(1 * (layer_size > 0)) * 8");

    /* == Edge BroadcastEpsilon[out_1] -> [fo_moment_in]optimizeWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastEpsilon,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(1 * (weights_size > 0)) * 8",
            /* = Sink vertex            = */  vertex_optimizeWeights,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(1 * (weights_size > 0)) * 8");

    /* == Edge BroadcastBetas[out_0] -> [param_in]optimizeBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastBetas,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(4 * (weights_size > 0)) * 8",
            /* = Sink vertex            = */  vertex_optimizeBias,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(4 * (layer_size > 0)) * 8");

    /* == Edge BroadcastBetas[out_1] -> [epsilon]optimizeWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastBetas,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(4 * (weights_size > 0)) * 8",
            /* = Sink vertex            = */  vertex_optimizeWeights,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(4 * (weights_size > 0)) * 8");

    /* == Edge betas[betas] -> [in]BroadcastBetas == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_betas,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(4) * 8",
            /* = Sink vertex            = */  vertex_BroadcastBetas,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(4 * (weights_size > 0)) * 8");

    /* == Edge bias[bias] -> [fo_moment_in]optimizeBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_bias,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(layer_size) * 4",
            /* = Sink vertex            = */  vertex_optimizeBias,
            /* = Sink port ix           = */  3,
            /* = sink rate expression   = */ "(layer_size) * 4");

    /* == Edge ForkGradients[out_1] -> [so_moment_in]optimizeWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkGradients,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(weights_size) * 4",
            /* = Sink vertex            = */  vertex_optimizeWeights,
            /* = Sink port ix           = */  6,
            /* = sink rate expression   = */ "(weights_size) * 4");

    /* == Edge ForkGradients[out_0] -> [learning_rate]optimizeBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkGradients,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(layer_size) * 4",
            /* = Sink vertex            = */  vertex_optimizeBias,
            /* = Sink port ix           = */  6,
            /* = sink rate expression   = */ "(layer_size) * 4");

    /* == Edge gradients[gradients] -> [in]ForkGradients == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_gradients,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(layer_size + weights_size) * 4",
            /* = Sink vertex            = */  vertex_ForkGradients,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(layer_size + weights_size) * 4");

    /* == Edge first_order_moments[first_order_moments] -> [in]ForkFirstOrder == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_first_order_moments,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(layer_size + weights_size) * 8",
            /* = Sink vertex            = */  vertex_ForkFirstOrder,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(layer_size + weights_size) * 8");

    /* == Edge ForkFirstOrder[out_0] -> [gradients]optimizeBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkFirstOrder,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(layer_size) * 8",
            /* = Sink vertex            = */  vertex_optimizeBias,
            /* = Sink port ix           = */  4,
            /* = sink rate expression   = */ "(layer_size) * 8");

    /* == Edge ForkFirstOrder[out_1] -> [param_in]optimizeWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkFirstOrder,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(weights_size) * 8",
            /* = Sink vertex            = */  vertex_optimizeWeights,
            /* = Sink port ix           = */  4,
            /* = sink rate expression   = */ "(weights_size) * 8");

    /* == Edge second_order_moments[second_order_moments] -> [in]ForkSecondOrder == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_second_order_moments,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(layer_size + weights_size) * 8",
            /* = Sink vertex            = */  vertex_ForkSecondOrder,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(layer_size + weights_size) * 8");

    /* == Edge ForkSecondOrder[out_0] -> [so_moment_in]optimizeBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkSecondOrder,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(layer_size) * 8",
            /* = Sink vertex            = */  vertex_optimizeBias,
            /* = Sink port ix           = */  5,
            /* = sink rate expression   = */ "(layer_size) * 8");

    /* == Edge ForkSecondOrder[out_1] -> [learning_rate]optimizeWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkSecondOrder,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(weights_size) * 8",
            /* = Sink vertex            = */  vertex_optimizeWeights,
            /* = Sink port ix           = */  5,
            /* = sink rate expression   = */ "(weights_size) * 8");

    /* == Edge JoinFirstOrder[out] -> [first_order_moments_out]first_order_moments_out == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_JoinFirstOrder,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(weights_size + layer_size) * 8",
            /* = Sink vertex            = */  vertex_first_order_moments_out,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(layer_size + weights_size) * 8");

    /* == Edge JoinSecondOrder[out] -> [second_order_moments_out]second_order_moments_out == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_JoinSecondOrder,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(layer_size + weights_size) * 8",
            /* = Sink vertex            = */  vertex_second_order_moments_out,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(layer_size + weights_size) * 8");

    /* == Edge optimizeBias[param_out] -> [in_0]JoinFirstOrder == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_optimizeBias,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(layer_size) * 8",
            /* = Sink vertex            = */  vertex_JoinFirstOrder,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(layer_size) * 8");

    /* == Edge optimizeBias[so_moment_out] -> [in_0]JoinSecondOrder == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_optimizeBias,
            /* = Source port ix         = */  2,
            /* = Source rate expression = */ "(layer_size) * 8",
            /* = Sink vertex            = */  vertex_JoinSecondOrder,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(layer_size) * 8");

    /* == Edge optimizeWeights[so_moment_out] -> [in_1]JoinFirstOrder == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_optimizeWeights,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(weights_size) * 8",
            /* = Sink vertex            = */  vertex_JoinFirstOrder,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(weights_size) * 8");

    /* == Edge optimizeWeights[param_out] -> [in_1]JoinSecondOrder == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_optimizeWeights,
            /* = Source port ix         = */  2,
            /* = Source rate expression = */ "(weights_size) * 8",
            /* = Sink vertex            = */  vertex_JoinSecondOrder,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(weights_size) * 8");

    /* == Return the graph as a Vertex == */
    return spider::api::convertGraphToVertex(graph);
}
