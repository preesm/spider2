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

/* === Include(s) === */

#include "spider2-reinforcement.h"

/* === Function(s) definition === */

spider::pisdf::Graph *spider::rl::createUserApplicationGraph() {
    /* == Create the graph == */
    auto *graph = spider::api::createGraph(
            /* = Name of the application graph = */ "training",
            /* = Number of actors              = */  57,
            /* = Number of edges               = */  55,
            /* = Number of parameters          = */  10,
            /* = Number of input interfaces    = */  0,
            /* = Number of output interfaces   = */  0,
            /* = Number of config actors       = */  0);

    /* === Creates the parameter(s) === */

    auto param_action_space_size = spider::api::createStaticParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "action_space_size",
            /* = Value of the parameter = */  1);

    auto param_state_angular_size = spider::api::createStaticParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "state_angular_size",
            /* = Value of the parameter = */  2);

    auto param_state_space_size = spider::api::createStaticParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "state_space_size",
            /* = Value of the parameter = */  3);

    auto param_value_space_size = spider::api::createStaticParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "value_space_size",
            /* = Value of the parameter = */  1);

    auto param_critic_hidden_size = spider::api::createStaticParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "critic_hidden_size",
            /* = Value of the parameter = */  20);

    auto param_actor_hidden_size = spider::api::createStaticParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "actor_hidden_size",
            /* = Value of the parameter = */  20);

    auto param_critic_weights_size = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "critic_weights_size",
            /* = Expression of the parameter = */
                                                   "state_space_size * critic_hidden_size + critic_hidden_size * value_space_size");

    auto param_critic_bias_size = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "critic_bias_size",
            /* = Expression of the parameter = */ "critic_hidden_size + value_space_size");

    auto param_actor_weights_size = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "actor_weights_size",
            /* = Expression of the parameter = */
                                                   "state_space_size * actor_hidden_size + actor_hidden_size * action_space_size");

    auto param_actor_bias_size = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "actor_bias_size",
            /* = Expression of the parameter = */ "action_space_size + actor_hidden_size");

    /* === Set the input interface(s) === */

    /* === Set the output interface(s) === */

    /* === Creates the actor(s) == */

    auto *vertex_BroadcastStateFeature = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastStateFeature",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  5,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastStateFeature == */
    spider::api::addInputParamsToVertex(vertex_BroadcastStateFeature, { param_state_space_size });

    /* == Setting mappable constraints of the vertex BroadcastStateFeature == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastStateFeature, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastStateFeature == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastStateFeature, TYPE_X86, "100");

    auto *vertex_BroadcastBiasActor = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastBiasActor",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastBiasActor == */
    spider::api::addInputParamsToVertex(vertex_BroadcastBiasActor, { param_actor_bias_size });

    /* == Setting mappable constraints of the vertex BroadcastBiasActor == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastBiasActor, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastBiasActor == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastBiasActor, TYPE_X86, "100");

    auto *vertex_renderEnv = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "renderEnv",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  0,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::RENDERENV);


    /* == Set the input parameters needed by the refinement of the vertex renderEnv == */
    spider::api::addInputRefinementParamToVertex(vertex_renderEnv, param_state_angular_size);

    /* == Setting mappable constraints of the vertex renderEnv == */
    spider::api::setVertexMappableOnPE(vertex_renderEnv, PE_X86_CORE0, true);

    /* == Set the timings of the vertex renderEnv == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_renderEnv, TYPE_X86, "100");

    auto *vertex_broadcastAction = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "broadcastAction",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex broadcastAction == */
    spider::api::addInputParamsToVertex(vertex_broadcastAction, { param_action_space_size });

    /* == Setting mappable constraints of the vertex broadcastAction == */
    spider::api::setVertexMappableOnPE(vertex_broadcastAction, PE_X86_CORE0, true);

    /* == Set the timings of the vertex broadcastAction == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_broadcastAction, TYPE_X86, "100");

    auto *vertex_gen_actor_learning_rate = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "gen_actor_learning_rate",
            /* = Number of input edges  = */  0,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::ACTORLEARNINGRATEGEN);


    /* == Setting mappable constraints of the vertex gen_actor_learning_rate == */
    spider::api::setVertexMappableOnPE(vertex_gen_actor_learning_rate, PE_X86_CORE0, true);

    /* == Set the timings of the vertex gen_actor_learning_rate == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_gen_actor_learning_rate, TYPE_X86, "100");

    auto *vertex_broadcastState = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "broadcastState",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex broadcastState == */
    spider::api::addInputParamsToVertex(vertex_broadcastState, { param_state_angular_size });

    /* == Setting mappable constraints of the vertex broadcastState == */
    spider::api::setVertexMappableOnPE(vertex_broadcastState, PE_X86_CORE0, true);

    /* == Set the timings of the vertex broadcastState == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_broadcastState, TYPE_X86, "100");

    auto *vertex_Temporal_Difference_Error = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "Temporal_Difference_Error",
            /* = Number of input edges  = */  3,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::TD_ERROR);


    /* == Setting mappable constraints of the vertex Temporal_Difference_Error == */
    spider::api::setVertexMappableOnPE(vertex_Temporal_Difference_Error, PE_X86_CORE0, true);

    /* == Set the timings of the vertex Temporal_Difference_Error == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_Temporal_Difference_Error, TYPE_X86, "100");

    auto *vertex_BroadcastWeightsActor = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastWeightsActor",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastWeightsActor == */
    spider::api::addInputParamsToVertex(vertex_BroadcastWeightsActor, { param_actor_weights_size });

    /* == Setting mappable constraints of the vertex BroadcastWeightsActor == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastWeightsActor, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastWeightsActor == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastWeightsActor, TYPE_X86, "100");

    auto *vertex_Environment = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "Environment",
            /* = Number of input edges  = */  2,
            /* = Number of output edges = */  3,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::STEP);


    /* == Set the input parameters needed by the refinement of the vertex Environment == */
    spider::api::addInputRefinementParamToVertex(vertex_Environment, param_state_space_size);
    spider::api::addInputRefinementParamToVertex(vertex_Environment, param_action_space_size);
    spider::api::addInputRefinementParamToVertex(vertex_Environment, param_state_angular_size);

    /* == Setting mappable constraints of the vertex Environment == */
    spider::api::setVertexMappableOnPE(vertex_Environment, PE_X86_CORE0, true);

    /* == Set the timings of the vertex Environment == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_Environment, TYPE_X86, "100");

    auto *vertex_BroadcastBiasCritic = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastBiasCritic",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  3,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastBiasCritic == */
    spider::api::addInputParamsToVertex(vertex_BroadcastBiasCritic, { param_critic_bias_size });

    /* == Setting mappable constraints of the vertex BroadcastBiasCritic == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastBiasCritic, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastBiasCritic == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastBiasCritic, TYPE_X86, "100");

    auto *vertex_GaussianPolicy = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "GaussianPolicy",
            /* = Number of input edges  = */  2,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::ACTIONSAMPLER);


    /* == Set the input parameters needed by the refinement of the vertex GaussianPolicy == */
    spider::api::addInputRefinementParamToVertex(vertex_GaussianPolicy, param_action_space_size);

    /* == Setting mappable constraints of the vertex GaussianPolicy == */
    spider::api::setVertexMappableOnPE(vertex_GaussianPolicy, PE_X86_CORE0, true);

    /* == Set the timings of the vertex GaussianPolicy == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_GaussianPolicy, TYPE_X86, "100");

    auto *vertex_gen_critic_learning_rate = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "gen_critic_learning_rate",
            /* = Number of input edges  = */  0,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::CRITICLEARNINGRATEGEN);


    /* == Setting mappable constraints of the vertex gen_critic_learning_rate == */
    spider::api::setVertexMappableOnPE(vertex_gen_critic_learning_rate, PE_X86_CORE0, true);

    /* == Set the timings of the vertex gen_critic_learning_rate == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_gen_critic_learning_rate, TYPE_X86, "100");

    auto *vertex_ClipAction = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "ClipAction",
            /* = Number of input edges  = */  2,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::CLIPVALUES);


    /* == Set the input parameters needed by the refinement of the vertex ClipAction == */
    spider::api::addInputRefinementParamToVertex(vertex_ClipAction, param_action_space_size);

    /* == Setting mappable constraints of the vertex ClipAction == */
    spider::api::setVertexMappableOnPE(vertex_ClipAction, PE_X86_CORE0, true);

    /* == Set the timings of the vertex ClipAction == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_ClipAction, TYPE_X86, "100");

    auto *vertex_gen_environment_limits = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "gen_environment_limits",
            /* = Number of input edges  = */  0,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::ENVACTIONLIMITS);


    /* == Setting mappable constraints of the vertex gen_environment_limits == */
    spider::api::setVertexMappableOnPE(vertex_gen_environment_limits, PE_X86_CORE0, true);

    /* == Set the timings of the vertex gen_environment_limits == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_gen_environment_limits, TYPE_X86, "100");

    auto *vertex_BroadcastWeightsCritic = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastWeightsCritic",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  3,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastWeightsCritic == */
    spider::api::addInputParamsToVertex(vertex_BroadcastWeightsCritic, { param_critic_weights_size });

    /* == Setting mappable constraints of the vertex BroadcastWeightsCritic == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastWeightsCritic, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastWeightsCritic == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastWeightsCritic, TYPE_X86, "100");

    auto *vertex_gen_sigma = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "gen_sigma",
            /* = Number of input edges  = */  0,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::SIGMAGEN);


    /* == Setting mappable constraints of the vertex gen_sigma == */
    spider::api::setVertexMappableOnPE(vertex_gen_sigma, PE_X86_CORE0, true);

    /* == Set the timings of the vertex gen_sigma == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_gen_sigma, TYPE_X86, "100");

    /* === Creates the subgraph(s) === */

    auto *vertex_Critic_MLP_Next = spider::rl::createMlpSubgraph("Critic_MLP_Next", graph,
                                                                 { param_value_space_size, param_critic_hidden_size,
                                                                   param_state_space_size });

    auto *vertex_Critic_MLP = spider::rl::createMlpSubgraph("Critic_MLP", graph,
                                                            { param_value_space_size, param_critic_hidden_size,
                                                              param_state_space_size });

    auto *vertex_Update_Critic = spider::rl::createNetwork_trainSubgraph("Update_Critic", graph,
                                                                         { param_state_space_size,
                                                                           param_critic_hidden_size,
                                                                           param_value_space_size });

    auto *vertex_Actor_MLP = spider::rl::createMlpSubgraph("Actor_MLP", graph,
                                                           { param_action_space_size, param_actor_hidden_size,
                                                             param_state_space_size });

    auto *vertex_ActorUpdate = spider::rl::createTrain_actorSubgraph("ActorUpdate", graph,
                                                                     { param_actor_hidden_size, param_action_space_size,
                                                                       param_state_space_size });

    /* === Creates the edge(s) === */

    /* == Edge broadcastState[state_out1] -> [input_actions]Environment == */
    auto *edge_broadcastState_state_out1__Environment_input_actions = spider::api::createEdge(
            /* = Source vertex          = */  vertex_broadcastState,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(state_angular_size) * 4",
            /* = Sink vertex            = */  vertex_Environment,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(state_angular_size) * 4");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_broadcastState_state_out1__Environment_input_actions,
            /* = Expression of the delay = */ "(state_angular_size) * 4");

    /* == Edge BroadcastStateFeature[state_out2] -> [input]Critic_MLP == */
    auto *edge_BroadcastStateFeature_state_out2__Critic_MLP_input = spider::api::createEdge(
            /* = Source vertex          = */  vertex_BroadcastStateFeature,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(state_space_size) * 4",
            /* = Sink vertex            = */  vertex_Critic_MLP,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(state_space_size) * 4");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_BroadcastStateFeature_state_out2__Critic_MLP_input,
            /* = Expression of the delay = */ "(state_space_size) * 4");

    /* == Edge BroadcastStateFeature[state_out1] -> [input]Actor_MLP == */
    auto *edge_BroadcastStateFeature_state_out1__Actor_MLP_input = spider::api::createEdge(
            /* = Source vertex          = */  vertex_BroadcastStateFeature,
            /* = Source port ix         = */  4,
            /* = Source rate expression = */ "(state_space_size) * 4",
            /* = Sink vertex            = */  vertex_Actor_MLP,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(state_space_size) * 4");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_BroadcastStateFeature_state_out1__Actor_MLP_input,
            /* = Expression of the delay = */ "(state_space_size) * 4");

    /* == Edge BroadcastStateFeature[state_out3] -> [inputs]Update_Critic == */
    auto *edge_BroadcastStateFeature_state_out3__Update_Critic_inputs = spider::api::createEdge(
            /* = Source vertex          = */  vertex_BroadcastStateFeature,
            /* = Source port ix         = */  2,
            /* = Source rate expression = */ "(state_space_size) * 4",
            /* = Sink vertex            = */  vertex_Update_Critic,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(state_space_size) * 4");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_BroadcastStateFeature_state_out3__Update_Critic_inputs,
            /* = Expression of the delay = */ "(state_space_size) * 4");

    /* == Edge Update_Critic[weights_out] -> [input]BroadcastWeightsCritic == */
    auto *edge_Update_Critic_weights_out__BroadcastWeightsCritic_input = spider::api::createEdge(
            /* = Source vertex          = */  vertex_Update_Critic,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */
                                              "(state_space_size * critic_hidden_size + critic_hidden_size * value_space_size) * 4",
            /* = Sink vertex            = */  vertex_BroadcastWeightsCritic,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(critic_weights_size) * 4");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_Update_Critic_weights_out__BroadcastWeightsCritic_input,
            /* = Expression of the delay = */ "(critic_weights_size) * 4");

    /* == Edge Update_Critic[bias_out] -> [input]BroadcastBiasCritic == */
    auto *edge_Update_Critic_bias_out__BroadcastBiasCritic_input = spider::api::createEdge(
            /* = Source vertex          = */  vertex_Update_Critic,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(critic_hidden_size + value_space_size) * 4",
            /* = Sink vertex            = */  vertex_BroadcastBiasCritic,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(critic_bias_size) * 4");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_Update_Critic_bias_out__BroadcastBiasCritic_input,
            /* = Expression of the delay = */ "(critic_bias_size) * 4");

    /* == Edge BroadcastStateFeature[state_out4] -> [inputs]ActorUpdate == */
    auto *edge_BroadcastStateFeature_state_out4__ActorUpdate_inputs = spider::api::createEdge(
            /* = Source vertex          = */  vertex_BroadcastStateFeature,
            /* = Source port ix         = */  3,
            /* = Source rate expression = */ "(state_space_size) * 4",
            /* = Sink vertex            = */  vertex_ActorUpdate,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(state_space_size) * 4");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_BroadcastStateFeature_state_out4__ActorUpdate_inputs,
            /* = Expression of the delay = */ "(state_space_size) * 4");

    /* == Edge ActorUpdate[weights_out] -> [input]BroadcastWeightsActor == */
    auto *edge_ActorUpdate_weights_out__BroadcastWeightsActor_input = spider::api::createEdge(
            /* = Source vertex          = */  vertex_ActorUpdate,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */
                                              "(state_space_size * actor_hidden_size + actor_hidden_size * action_space_size) * 4",
            /* = Sink vertex            = */  vertex_BroadcastWeightsActor,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(actor_weights_size) * 4");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_ActorUpdate_weights_out__BroadcastWeightsActor_input,
            /* = Expression of the delay = */ "(actor_weights_size) * 4");

    /* == Edge ActorUpdate[bias_out] -> [input]BroadcastBiasActor == */
    auto *edge_ActorUpdate_bias_out__BroadcastBiasActor_input = spider::api::createEdge(
            /* = Source vertex          = */  vertex_ActorUpdate,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(action_space_size + actor_hidden_size) * 4",
            /* = Sink vertex            = */  vertex_BroadcastBiasActor,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(actor_bias_size) * 4");

    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */
            edge_ActorUpdate_bias_out__BroadcastBiasActor_input,
            /* = Expression of the delay = */ "(actor_bias_size) * 4");

    /* == Edge broadcastState[state_out0] -> [state]renderEnv == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_broadcastState,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(state_angular_size) * 4",
            /* = Sink vertex            = */  vertex_renderEnv,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(state_angular_size) * 4");

    /* == Edge Environment[state_observation] -> [state]broadcastState == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Environment,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(state_angular_size) * 4",
            /* = Sink vertex            = */  vertex_broadcastState,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(state_angular_size) * 4");

    /* == Edge Environment[state_angular_out] -> [state]BroadcastStateFeature == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Environment,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(state_space_size) * 4",
            /* = Sink vertex            = */  vertex_BroadcastStateFeature,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(state_space_size) * 4");

    /* == Edge BroadcastStateFeature[state_out0] -> [input]Critic_MLP_Next == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastStateFeature,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(state_space_size) * 4",
            /* = Sink vertex            = */  vertex_Critic_MLP_Next,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(state_space_size) * 4");

    /* == Edge Critic_MLP_Next[output] -> [value_state]Temporal_Difference_Error == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Critic_MLP_Next,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(value_space_size) * 4",
            /* = Sink vertex            = */  vertex_Temporal_Difference_Error,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge Environment[reward] -> [value_next_state]Temporal_Difference_Error == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Environment,
            /* = Source port ix         = */  2,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_Temporal_Difference_Error,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge Critic_MLP[output] -> [reward]Temporal_Difference_Error == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Critic_MLP,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(value_space_size) * 4",
            /* = Sink vertex            = */  vertex_Temporal_Difference_Error,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge broadcastAction[action_out0] -> [state_angular_in]Environment == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_broadcastAction,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(action_space_size) * 4",
            /* = Sink vertex            = */  vertex_Environment,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(action_space_size) * 4");

    /* == Edge Actor_MLP[output] -> [sigma_in]GaussianPolicy == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Actor_MLP,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(action_space_size) * 4",
            /* = Sink vertex            = */  vertex_GaussianPolicy,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(action_space_size) * 4");

    /* == Edge Temporal_Difference_Error[delta] -> [targets]Update_Critic == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Temporal_Difference_Error,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_Update_Critic,
            /* = Sink port ix           = */  3,
            /* = sink rate expression   = */ "(value_space_size) * 4");

    /* == Edge gen_sigma[sigma] -> [action_in]GaussianPolicy == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_gen_sigma,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_GaussianPolicy,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge gen_critic_learning_rate[learning_rate] -> [learning_rate]Update_Critic == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_gen_critic_learning_rate,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_Update_Critic,
            /* = Sink port ix           = */  4,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge GaussianPolicy[action_out] -> [input]ClipAction == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_GaussianPolicy,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(action_space_size) * 4",
            /* = Sink vertex            = */  vertex_ClipAction,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(action_space_size) * 4");

    /* == Edge ClipAction[output] -> [action]broadcastAction == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ClipAction,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(action_space_size) * 4",
            /* = Sink vertex            = */  vertex_broadcastAction,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(action_space_size) * 4");

    /* == Edge gen_environment_limits[limits] -> [limits]ClipAction == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_gen_environment_limits,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(2) * 4",
            /* = Sink vertex            = */  vertex_ClipAction,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(2) * 4");

    /* == Edge BroadcastBiasActor[out_0] -> [bias]Actor_MLP == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastBiasActor,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(actor_bias_size) * 4",
            /* = Sink vertex            = */  vertex_Actor_MLP,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(actor_hidden_size + action_space_size) * 4");

    /* == Edge BroadcastWeightsActor[out_1] -> [weights]Actor_MLP == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastWeightsActor,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(actor_weights_size) * 4",
            /* = Sink vertex            = */  vertex_Actor_MLP,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */
                                                              "(state_space_size * actor_hidden_size + actor_hidden_size * action_space_size) * 4");

    /* == Edge BroadcastWeightsCritic[out_2] -> [weights]Update_Critic == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastWeightsCritic,
            /* = Source port ix         = */  2,
            /* = Source rate expression = */ "(critic_weights_size) * 4",
            /* = Sink vertex            = */  vertex_Update_Critic,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */
                                                              "(state_space_size * critic_hidden_size + critic_hidden_size * value_space_size) * 4");

    /* == Edge BroadcastWeightsCritic[out_1] -> [weights]Critic_MLP == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastWeightsCritic,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(critic_weights_size) * 4",
            /* = Sink vertex            = */  vertex_Critic_MLP,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */
                                                              "(state_space_size * critic_hidden_size  + critic_hidden_size * value_space_size) * 4");

    /* == Edge BroadcastWeightsCritic[out_0] -> [weights]Critic_MLP_Next == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastWeightsCritic,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(critic_weights_size) * 4",
            /* = Sink vertex            = */  vertex_Critic_MLP_Next,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */
                                                              "(state_space_size* critic_hidden_size  + critic_hidden_size * value_space_size) * 4");

    /* == Edge BroadcastBiasCritic[out_2] -> [bias]Update_Critic == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastBiasCritic,
            /* = Source port ix         = */  2,
            /* = Source rate expression = */ "(critic_bias_size) * 4",
            /* = Sink vertex            = */  vertex_Update_Critic,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(value_space_size + critic_hidden_size) * 4");

    /* == Edge BroadcastBiasCritic[out_1] -> [bias]Critic_MLP == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastBiasCritic,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(critic_bias_size) * 4",
            /* = Sink vertex            = */  vertex_Critic_MLP,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(critic_hidden_size + value_space_size) * 4");

    /* == Edge BroadcastBiasCritic[out_0] -> [bias]Critic_MLP_Next == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastBiasCritic,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(critic_bias_size) * 4",
            /* = Sink vertex            = */  vertex_Critic_MLP_Next,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(critic_hidden_size + value_space_size) * 4");

    /* == Edge Temporal_Difference_Error[target] -> [delta]ActorUpdate == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Temporal_Difference_Error,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_ActorUpdate,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge broadcastAction[action_out1] -> [targets]ActorUpdate == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_broadcastAction,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(action_space_size) * 4",
            /* = Sink vertex            = */  vertex_ActorUpdate,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(action_space_size) * 4");

    /* == Edge gen_actor_learning_rate[learning_rate] -> [learning_rate]ActorUpdate == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_gen_actor_learning_rate,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_ActorUpdate,
            /* = Sink port ix           = */  3,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge BroadcastWeightsActor[out_0] -> [weights]ActorUpdate == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastWeightsActor,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(actor_weights_size) * 4",
            /* = Sink vertex            = */  vertex_ActorUpdate,
            /* = Sink port ix           = */  5,
            /* = sink rate expression   = */
                                                              "(state_space_size * actor_hidden_size + actor_hidden_size * action_space_size) * 4");

    /* == Edge BroadcastBiasActor[out_1] -> [bias]ActorUpdate == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastBiasActor,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(actor_bias_size) * 4",
            /* = Sink vertex            = */  vertex_ActorUpdate,
            /* = Sink port ix           = */  4,
            /* = sink rate expression   = */ "(action_space_size + actor_hidden_size) * 4");
    return graph;
}
