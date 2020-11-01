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

spider::pisdf::Vertex *spider::rl::createNetwork_trainSubgraph(std::string name,
                                                               spider::pisdf::Graph *parentGraph,
                                                               const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams) {
    /* == Create the subgraph == */
    auto *graph = spider::api::createSubgraph(parentGraph,
            /* = Name of the subgraph        = */ std::move(name),
            /* = Number of actors            = */ 48,
            /* = Number of edges             = */ 58,
            /* = Number of parameters        = */ 7,
            /* = Number of input interfaces  = */ 5,
            /* = Number of output interfaces = */ 2,
            /* = Number of config actors     = */ 0);

    /* === Creates the parameter(s) === */

    auto param_input_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "input_size",
            /* = Parent parameter       = */  parentGraphParams[0]);

    auto param_hidden_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "hidden_size",
            /* = Parent parameter       = */  parentGraphParams[1]);

    auto param_output_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "output_size",
            /* = Parent parameter       = */  parentGraphParams[2]);

    auto param_weights_size = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "weights_size",
            /* = Expression of the parameter = */ "input_size * hidden_size + hidden_size * output_size");

    auto param_bias_size = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "bias_size",
            /* = Expression of the parameter = */ "hidden_size + output_size");

    auto param_hidden_weights_size = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "hidden_weights_size",
            /* = Expression of the parameter = */ "input_size * hidden_size");

    auto param_output_weights_size = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "output_weights_size",
            /* = Expression of the parameter = */ "hidden_size * output_size");

    /* === Set the input interface(s) === */

    auto *vertex_weights = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "weights");

    auto *vertex_bias = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  1,
            /* = Name of the interface  = */ "bias");

    auto *vertex_inputs = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  2,
            /* = Name of the interface  = */ "inputs");

    auto *vertex_targets = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  3,
            /* = Name of the interface  = */ "targets");

    auto *vertex_learning_rate = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  4,
            /* = Name of the interface  = */ "learning_rate");

    /* === Set the output interface(s) === */

    auto *vertex_weights_out = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "weights_out");

    auto *vertex_bias_out = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  1,
            /* = Name of the interface  = */ "bias_out");

    /* === Creates the actor(s) == */

    auto *vertex_JoinWeights = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "JoinWeights",
            /* = Number of input edges  = */  2,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::JOIN,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex JoinWeights == */
    spider::api::addInputParamsToVertex(vertex_JoinWeights,
                                        { param_weights_size, param_hidden_weights_size, param_output_weights_size });

    /* == Setting mappable constraints of the vertex JoinWeights == */
    spider::api::setVertexMappableOnPE(vertex_JoinWeights, PE_X86_CORE0, true);

    /* == Set the timings of the vertex JoinWeights == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_JoinWeights, TYPE_X86, "100");

    auto *vertex_BroadcastBias = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastBias",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastBias == */
    spider::api::addInputParamsToVertex(vertex_BroadcastBias, { param_bias_size });

    /* == Setting mappable constraints of the vertex BroadcastBias == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastBias, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastBias == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastBias, TYPE_X86, "100");

    auto *vertex_JoinGradients_hidden = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "JoinGradients_hidden",
            /* = Number of input edges  = */  2,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::JOIN,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex JoinGradients_hidden == */
    spider::api::addInputParamsToVertex(vertex_JoinGradients_hidden, { param_hidden_weights_size, param_hidden_size });

    /* == Setting mappable constraints of the vertex JoinGradients_hidden == */
    spider::api::setVertexMappableOnPE(vertex_JoinGradients_hidden, PE_X86_CORE0, true);

    /* == Set the timings of the vertex JoinGradients_hidden == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_JoinGradients_hidden, TYPE_X86, "100");

    auto *vertex_BroadcastWeights = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastWeights",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastWeights == */
    spider::api::addInputParamsToVertex(vertex_BroadcastWeights, { param_weights_size });

    /* == Setting mappable constraints of the vertex BroadcastWeights == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastWeights, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastWeights == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastWeights, TYPE_X86, "100");

    auto *vertex_BroadcastInput = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastInput",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastInput == */
    spider::api::addInputParamsToVertex(vertex_BroadcastInput, { param_input_size });

    /* == Setting mappable constraints of the vertex BroadcastInput == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastInput, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastInput == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastInput, TYPE_X86, "100");

    auto *vertex_JoinGradients_output = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "JoinGradients_output",
            /* = Number of input edges  = */  2,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::JOIN,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex JoinGradients_output == */
    spider::api::addInputParamsToVertex(vertex_JoinGradients_output, { param_hidden_size, param_output_size });

    /* == Setting mappable constraints of the vertex JoinGradients_output == */
    spider::api::setVertexMappableOnPE(vertex_JoinGradients_output, PE_X86_CORE0, true);

    /* == Set the timings of the vertex JoinGradients_output == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_JoinGradients_output, TYPE_X86, "100");

    auto *vertex_BroadcastWeights_output = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastWeights_output",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastWeights_output == */
    spider::api::addInputParamsToVertex(vertex_BroadcastWeights_output, { param_output_weights_size });

    /* == Setting mappable constraints of the vertex BroadcastWeights_output == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastWeights_output, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastWeights_output == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastWeights_output, TYPE_X86, "100");

    auto *vertex_gen_epsilon = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "gen_epsilon",
            /* = Number of input edges  = */  0,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::ADAMEPSILONGEN);


    /* == Setting mappable constraints of the vertex gen_epsilon == */
    spider::api::setVertexMappableOnPE(vertex_gen_epsilon, PE_X86_CORE0, true);

    /* == Set the timings of the vertex gen_epsilon == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_gen_epsilon, TYPE_X86, "100");

    auto *vertex_BroadcastEpsilon = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastEpsilon",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Setting mappable constraints of the vertex BroadcastEpsilon == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastEpsilon, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastEpsilon == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastEpsilon, TYPE_X86, "100");

    auto *vertex_ForkBias = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "ForkBias",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::FORK,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex ForkBias == */
    spider::api::addInputParamsToVertex(vertex_ForkBias, { param_hidden_size, param_output_size, param_bias_size });

    /* == Setting mappable constraints of the vertex ForkBias == */
    spider::api::setVertexMappableOnPE(vertex_ForkBias, PE_X86_CORE0, true);

    /* == Set the timings of the vertex ForkBias == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_ForkBias, TYPE_X86, "100");

    auto *vertex_JoinBias = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "JoinBias",
            /* = Number of input edges  = */  2,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::JOIN,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex JoinBias == */
    spider::api::addInputParamsToVertex(vertex_JoinBias, { param_bias_size, param_hidden_size, param_output_size });

    /* == Setting mappable constraints of the vertex JoinBias == */
    spider::api::setVertexMappableOnPE(vertex_JoinBias, PE_X86_CORE0, true);

    /* == Set the timings of the vertex JoinBias == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_JoinBias, TYPE_X86, "100");

    auto *vertex_BroadcastBetas = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastBetas",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  3,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Setting mappable constraints of the vertex BroadcastBetas == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastBetas, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastBetas == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastBetas, TYPE_X86, "100");

    auto *vertex_adamBetas = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "adamBetas",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::ADAMUPDATEBETAS);


    /* == Setting mappable constraints of the vertex adamBetas == */
    spider::api::setVertexMappableOnPE(vertex_adamBetas, PE_X86_CORE0, true);

    /* == Set the timings of the vertex adamBetas == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_adamBetas, TYPE_X86, "100");

    auto *vertex_ForkWeights = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "ForkWeights",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::FORK,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex ForkWeights == */
    spider::api::addInputParamsToVertex(vertex_ForkWeights,
                                        { param_hidden_weights_size, param_output_weights_size, param_weights_size });

    /* == Setting mappable constraints of the vertex ForkWeights == */
    spider::api::setVertexMappableOnPE(vertex_ForkWeights, PE_X86_CORE0, true);

    /* == Set the timings of the vertex ForkWeights == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_ForkWeights, TYPE_X86, "100");

    auto *vertex_BroadcastErrors_output = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastErrors_output",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastErrors_output == */
    spider::api::addInputParamsToVertex(vertex_BroadcastErrors_output, { param_output_size });

    /* == Setting mappable constraints of the vertex BroadcastErrors_output == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastErrors_output, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastErrors_output == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastErrors_output, TYPE_X86, "100");

    auto *vertex_BroadcastLearningRate = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastLearningRate",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Setting mappable constraints of the vertex BroadcastLearningRate == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastLearningRate, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastLearningRate == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastLearningRate, TYPE_X86, "100");

    /* === Creates the subgraph(s) === */

    auto *vertex_AdamHidden = spider::rl::createAdamSubgraph("AdamHidden", graph,
                                                             { param_hidden_size, param_input_size });

    auto *vertex_GradientsHidden = spider::rl::createLayer_gradientsSubgraph("GradientsHidden", graph,
                                                                             { param_hidden_size, param_output_size,
                                                                               param_input_size });

    auto *vertex_GradientsOutput = spider::rl::createOutput_gradientsSubgraph("GradientsOutput", graph,
                                                                              { param_output_size, param_hidden_size });

    auto *vertex_MLP = spider::rl::createMlp_rawSubgraph("MLP", graph,
                                                         { param_output_size, param_hidden_size, param_input_size });

    auto *vertex_AdamOutput = spider::rl::createAdamSubgraph("AdamOutput", graph,
                                                             { param_output_size, param_hidden_size });

    /* === Creates the edge(s) === */

    /* == Edge AdamHidden[second_order_moments_out] -> [second_order_moments]AdamHidden == */
    auto *edge_AdamHidden_second_order_moments_out__AdamHidden_second_order_moments = spider::api::createEdge(
            /* = Source vertex          = */  vertex_AdamHidden,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(input_size * hidden_size + hidden_size) * 8",
            /* = Sink vertex            = */  vertex_AdamHidden,
            /* = Sink port ix           = */  4,
            /* = sink rate expression   = */ "(input_size * hidden_size + hidden_size) * 8");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_AdamHidden_second_order_moments_out__AdamHidden_second_order_moments,
            /* = Expression of the delay = */ "(hidden_weights_size + hidden_size) * 8");

    /* == Edge AdamHidden[first_order_moments_out] -> [first_order_moments]AdamHidden == */
    auto *edge_AdamHidden_first_order_moments_out__AdamHidden_first_order_moments = spider::api::createEdge(
            /* = Source vertex          = */  vertex_AdamHidden,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(input_size * hidden_size + hidden_size) * 8",
            /* = Sink vertex            = */  vertex_AdamHidden,
            /* = Sink port ix           = */  3,
            /* = sink rate expression   = */ "(input_size * hidden_size + hidden_size) * 8");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_AdamHidden_first_order_moments_out__AdamHidden_first_order_moments,
            /* = Expression of the delay = */ "(hidden_weights_size + hidden_size) * 8");

    /* == Edge AdamOutput[first_order_moments_out] -> [first_order_moments]AdamOutput == */
    auto *edge_AdamOutput_first_order_moments_out__AdamOutput_first_order_moments = spider::api::createEdge(
            /* = Source vertex          = */  vertex_AdamOutput,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(hidden_size * output_size + output_size) * 8",
            /* = Sink vertex            = */  vertex_AdamOutput,
            /* = Sink port ix           = */  3,
            /* = sink rate expression   = */ "(hidden_size * output_size + output_size) * 8");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_AdamOutput_first_order_moments_out__AdamOutput_first_order_moments,
            /* = Expression of the delay = */ "(output_weights_size + output_size) * 8");

    /* == Edge AdamOutput[second_order_moments_out] -> [second_order_moments]AdamOutput == */
    auto *edge_AdamOutput_second_order_moments_out__AdamOutput_second_order_moments = spider::api::createEdge(
            /* = Source vertex          = */  vertex_AdamOutput,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(hidden_size * output_size + output_size) * 8",
            /* = Sink vertex            = */  vertex_AdamOutput,
            /* = Sink port ix           = */  4,
            /* = sink rate expression   = */ "(hidden_size * output_size + output_size) * 8");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_AdamOutput_second_order_moments_out__AdamOutput_second_order_moments,
            /* = Expression of the delay = */ "(output_weights_size + output_size) * 8");

    /* == Edge BroadcastBetas[out_2] -> [betas_in]adamBetas == */
    auto *edge_BroadcastBetas_out_2__adamBetas_betas_in = spider::api::createEdge(
            /* = Source vertex          = */  vertex_BroadcastBetas,
            /* = Source port ix         = */  2,
            /* = Source rate expression = */ "(4) * 8",
            /* = Sink vertex            = */  vertex_adamBetas,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(4) * 8");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */  edge_BroadcastBetas_out_2__adamBetas_betas_in,
            /* = Expression of the delay = */ "(4) * 8");

    /* == Edge JoinWeights[out] -> [weights_out]weights_out == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_JoinWeights,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(weights_size) * 4",
            /* = Sink vertex            = */  vertex_weights_out,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(weights_size) * 4");

    /* == Edge JoinBias[out] -> [bias_out]bias_out == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_JoinBias,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(bias_size) * 4",
            /* = Sink vertex            = */  vertex_bias_out,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(bias_size) * 4");

    /* == Edge AdamHidden[weights_out] -> [in_0]JoinWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_AdamHidden,
            /* = Source port ix         = */  2,
            /* = Source rate expression = */ "(input_size * hidden_size) * 4",
            /* = Sink vertex            = */  vertex_JoinWeights,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(hidden_weights_size) * 4");

    /* == Edge AdamHidden[bias_out] -> [in_0]JoinBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_AdamHidden,
            /* = Source port ix         = */  3,
            /* = Source rate expression = */ "(hidden_size) * 4",
            /* = Sink vertex            = */  vertex_JoinBias,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(hidden_size) * 4");

    /* == Edge AdamOutput[weights_out] -> [in_1]JoinWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_AdamOutput,
            /* = Source port ix         = */  2,
            /* = Source rate expression = */ "(hidden_size * output_size) * 4",
            /* = Sink vertex            = */  vertex_JoinWeights,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(output_weights_size) * 4");

    /* == Edge AdamOutput[bias_out] -> [in_1]JoinBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_AdamOutput,
            /* = Source port ix         = */  3,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_JoinBias,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge MLP[output] -> [output]GradientsOutput == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_MLP,
            /* = Source port ix         = */  3,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_GradientsOutput,
            /* = Sink port ix           = */  3,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge MLP[raw_output] -> [raw_output]GradientsOutput == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_MLP,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_GradientsOutput,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge MLP[raw_hidden] -> [raw_hidden]GradientsHidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_MLP,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(hidden_size) * 4",
            /* = Sink vertex            = */  vertex_GradientsHidden,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(hidden_size) * 4");

    /* == Edge MLP[hidden] -> [inputs]GradientsOutput == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_MLP,
            /* = Source port ix         = */  2,
            /* = Source rate expression = */ "(hidden_size) * 4",
            /* = Sink vertex            = */  vertex_GradientsOutput,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(hidden_size) * 4");

    /* == Edge BroadcastWeights[out_1] -> [weights]MLP == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastWeights,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(weights_size) * 4",
            /* = Sink vertex            = */  vertex_MLP,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(input_size * hidden_size + hidden_size * output_size) * 4");

    /* == Edge weights[weights] -> [in]BroadcastWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_weights,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(weights_size) * 4",
            /* = Sink vertex            = */  vertex_BroadcastWeights,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(weights_size) * 4");

    /* == Edge BroadcastWeights[out_0] -> [in]ForkWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastWeights,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(weights_size) * 4",
            /* = Sink vertex            = */  vertex_ForkWeights,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(weights_size) * 4");

    /* == Edge bias[bias] -> [in]BroadcastBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_bias,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(bias_size) * 4",
            /* = Sink vertex            = */  vertex_BroadcastBias,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(bias_size) * 4");

    /* == Edge BroadcastBias[out_1] -> [bias]MLP == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastBias,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(bias_size) * 4",
            /* = Sink vertex            = */  vertex_MLP,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(hidden_size + output_size) * 4");

    /* == Edge BroadcastBias[out_0] -> [in]ForkBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastBias,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(bias_size) * 4",
            /* = Sink vertex            = */  vertex_ForkBias,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(bias_size) * 4");

    /* == Edge ForkWeights[out_0] -> [weights]AdamHidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkWeights,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(hidden_weights_size) * 4",
            /* = Sink vertex            = */  vertex_AdamHidden,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(input_size * hidden_size) * 4");

    /* == Edge ForkBias[out_0] -> [bias]AdamHidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkBias,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(hidden_size) * 4",
            /* = Sink vertex            = */  vertex_AdamHidden,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(hidden_size) * 4");

    /* == Edge ForkBias[out_1] -> [bias]AdamOutput == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkBias,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_AdamOutput,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge BroadcastInput[out_0] -> [input]MLP == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastInput,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(input_size) * 4",
            /* = Sink vertex            = */  vertex_MLP,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(input_size) * 4");

    /* == Edge BroadcastInput[out_1] -> [inputs]GradientsHidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastInput,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(input_size) * 4",
            /* = Sink vertex            = */  vertex_GradientsHidden,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(input_size) * 4");

    /* == Edge inputs[inputs] -> [in]BroadcastInput == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_inputs,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(input_size) * 4",
            /* = Sink vertex            = */  vertex_BroadcastInput,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(input_size) * 4");

    /* == Edge BroadcastWeights_output[out_0] -> [next_layer_weights]GradientsHidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastWeights_output,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(output_weights_size) * 4",
            /* = Sink vertex            = */  vertex_GradientsHidden,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(hidden_size * output_size) * 4");

    /* == Edge BroadcastWeights_output[out_1] -> [weights]AdamOutput == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastWeights_output,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(output_weights_size) * 4",
            /* = Sink vertex            = */  vertex_AdamOutput,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(hidden_size * output_size) * 4");

    /* == Edge ForkWeights[out_1] -> [in]BroadcastWeights_output == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkWeights,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(output_weights_size) * 4",
            /* = Sink vertex            = */  vertex_BroadcastWeights_output,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(output_weights_size) * 4");

    /* == Edge JoinGradients_hidden[out] -> [gradients]AdamHidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_JoinGradients_hidden,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(hidden_size + hidden_weights_size) * 4",
            /* = Sink vertex            = */  vertex_AdamHidden,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(input_size * hidden_size + hidden_size) * 4");

    /* == Edge GradientsOutput[weights_gradient] -> [in_0]JoinGradients_output == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_GradientsOutput,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(hidden_size * output_size) * 4",
            /* = Sink vertex            = */  vertex_JoinGradients_output,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(hidden_size * output_size) * 4");

    /* == Edge JoinGradients_output[out] -> [gradients]AdamOutput == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_JoinGradients_output,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(hidden_size * output_size + output_size) * 4",
            /* = Sink vertex            = */  vertex_AdamOutput,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(hidden_size * output_size + output_size) * 4");

    /* == Edge GradientsOutput[bias_gradient] -> [in]BroadcastErrors_output == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_GradientsOutput,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_BroadcastErrors_output,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge BroadcastErrors_output[out_1] -> [in_1]JoinGradients_output == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastErrors_output,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_JoinGradients_output,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge BroadcastErrors_output[out_0] -> [next_layer_errors]GradientsHidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastErrors_output,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_GradientsHidden,
            /* = Sink port ix           = */  3,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge targets[targets] -> [target]GradientsOutput == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_targets,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_GradientsOutput,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge learning_rate[learning_rate] -> [in]BroadcastLearningRate == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_learning_rate,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_BroadcastLearningRate,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge BroadcastLearningRate[out_0] -> [learning_rate]AdamOutput == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastLearningRate,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_AdamOutput,
            /* = Sink port ix           = */  7,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge BroadcastLearningRate[out_1] -> [learning_rate]AdamHidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastLearningRate,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_AdamHidden,
            /* = Sink port ix           = */  7,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge gen_epsilon[epsilon] -> [in]BroadcastEpsilon == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_gen_epsilon,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 8",
            /* = Sink vertex            = */  vertex_BroadcastEpsilon,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1) * 8");

    /* == Edge BroadcastEpsilon[out_1] -> [epsilon]AdamOutput == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastEpsilon,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 8",
            /* = Sink vertex            = */  vertex_AdamOutput,
            /* = Sink port ix           = */  6,
            /* = sink rate expression   = */ "(1) * 8");

    /* == Edge BroadcastEpsilon[out_0] -> [epsilon]AdamHidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastEpsilon,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(1) * 8",
            /* = Sink vertex            = */  vertex_AdamHidden,
            /* = Sink port ix           = */  6,
            /* = sink rate expression   = */ "(1) * 8");

    /* == Edge adamBetas[betas_out] -> [in]BroadcastBetas == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_adamBetas,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(4) * 8",
            /* = Sink vertex            = */  vertex_BroadcastBetas,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(4) * 8");

    /* == Edge BroadcastBetas[out_1] -> [betas]AdamHidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastBetas,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(4) * 8",
            /* = Sink vertex            = */  vertex_AdamHidden,
            /* = Sink port ix           = */  5,
            /* = sink rate expression   = */ "(4) * 8");

    /* == Edge BroadcastBetas[out_0] -> [betas]AdamOutput == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastBetas,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(4) * 8",
            /* = Sink vertex            = */  vertex_AdamOutput,
            /* = Sink port ix           = */  5,
            /* = sink rate expression   = */ "(4) * 8");

    /* == Edge GradientsHidden[weights_gradient] -> [in_1]JoinGradients_hidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_GradientsHidden,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(input_size * hidden_size) * 4",
            /* = Sink vertex            = */  vertex_JoinGradients_hidden,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(hidden_weights_size) * 4");

    /* == Edge GradientsHidden[bias_gradient] -> [in_0]JoinGradients_hidden == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_GradientsHidden,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(hidden_size) * 4",
            /* = Sink vertex            = */  vertex_JoinGradients_hidden,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(hidden_size) * 4");

    /* == Return the graph as a Vertex == */
    return spider::api::convertGraphToVertex(graph);
}
