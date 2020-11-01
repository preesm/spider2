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

spider::pisdf::Vertex *spider::rl::createMlpSubgraph(std::string name,
                                                     spider::pisdf::Graph *parentGraph,
                                                     const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams) {
    /* == Create the subgraph == */
    auto *graph = spider::api::createSubgraph(parentGraph,
            /* = Name of the subgraph        = */ std::move(name),
            /* = Number of actors            = */ 10,
            /* = Number of edges             = */ 11,
            /* = Number of parameters        = */ 5,
            /* = Number of input interfaces  = */ 3,
            /* = Number of output interfaces = */ 1,
            /* = Number of config actors     = */ 0);

    /* === Creates the parameter(s) === */

    auto param_output_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "output_size",
            /* = Parent parameter       = */  parentGraphParams[0]);

    auto param_hidden_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "hidden_size",
            /* = Parent parameter       = */  parentGraphParams[1]);

    auto param_input_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "input_size",
            /* = Parent parameter       = */  parentGraphParams[2]);

    auto param_size_weights_hidden = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "size_weights_hidden",
            /* = Expression of the parameter = */ "input_size * hidden_size");

    auto param_size_weights_output = spider::api::createDerivedParam(
            /* = Graph of the parameter      = */  graph,
            /* = Name of the parameter       = */ "size_weights_output",
            /* = Expression of the parameter = */ "output_size * hidden_size");

    /* === Set the input interface(s) === */

    auto *vertex_input = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "input");

    auto *vertex_weights = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  1,
            /* = Name of the interface  = */ "weights");

    auto *vertex_bias = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  2,
            /* = Name of the interface  = */ "bias");

    /* === Set the output interface(s) === */

    auto *vertex_output = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "output");

    /* === Creates the actor(s) == */

    auto *vertex_ForkBias = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "ForkBias",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::FORK,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex ForkBias == */
    spider::api::addInputParamsToVertex(vertex_ForkBias, { param_hidden_size, param_output_size });

    /* == Setting mappable constraints of the vertex ForkBias == */
    spider::api::setVertexMappableOnPE(vertex_ForkBias, PE_X86_CORE0, true);

    /* == Set the timings of the vertex ForkBias == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_ForkBias, TYPE_X86, "100");

    auto *vertex_ForkWeights = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "ForkWeights",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::FORK,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex ForkWeights == */
    spider::api::addInputParamsToVertex(vertex_ForkWeights, { param_size_weights_hidden, param_size_weights_output });

    /* == Setting mappable constraints of the vertex ForkWeights == */
    spider::api::setVertexMappableOnPE(vertex_ForkWeights, PE_X86_CORE0, true);

    /* == Set the timings of the vertex ForkWeights == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_ForkWeights, TYPE_X86, "100");

    auto *vertex_activationFunction = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "activationFunction",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::ACTIVATETANHYPERBOLIC);


    /* == Setting mappable constraints of the vertex activationFunction == */
    spider::api::setVertexMappableOnPE(vertex_activationFunction, PE_X86_CORE0, true);

    /* == Set the timings of the vertex activationFunction == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_activationFunction, TYPE_X86, "100");

    auto *vertex_outputActivation = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "outputActivation",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::ACTIVATELINEAR);


    /* == Setting mappable constraints of the vertex outputActivation == */
    spider::api::setVertexMappableOnPE(vertex_outputActivation, PE_X86_CORE0, true);

    /* == Set the timings of the vertex outputActivation == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_outputActivation, TYPE_X86, "100");

    /* === Creates the subgraph(s) === */

    auto *vertex_Hidden_layer = spider::rl::createNeuron_novalidSubgraph("Hidden_layer", graph,
                                                                         { param_input_size, param_hidden_size });

    auto *vertex_Output_layer = spider::rl::createNeuron_novalidSubgraph("Output_layer", graph,
                                                                         { param_hidden_size, param_output_size });

    /* === Creates the edge(s) === */

    /* == Edge input[input] -> [input]Hidden_layer == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_input,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(input_size) * 4",
            /* = Sink vertex            = */  vertex_Hidden_layer,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(input_size) * 4");

    /* == Edge ForkBias[bias_out0] -> [bias_values]Hidden_layer == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkBias,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(hidden_size) * 4",
            /* = Sink vertex            = */  vertex_Hidden_layer,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(hidden_size) * 4");

    /* == Edge ForkBias[bias_out1] -> [bias_values]Output_layer == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkBias,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_Output_layer,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge bias[bias] -> [bias_in]ForkBias == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_bias,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(hidden_size + output_size) * 4",
            /* = Sink vertex            = */  vertex_ForkBias,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(hidden_size + output_size) * 4");

    /* == Edge Hidden_layer[output] -> [input]activationFunction == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Hidden_layer,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(hidden_size) * 4",
            /* = Sink vertex            = */  vertex_activationFunction,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge activationFunction[output] -> [input]Output_layer == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_activationFunction,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_Output_layer,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(hidden_size) * 4");

    /* == Edge outputActivation[output] -> [output]output == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_outputActivation,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_output,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge Output_layer[output] -> [input]outputActivation == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Output_layer,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_outputActivation,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Edge weights[weights] -> [weights_in]ForkWeights == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_weights,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(size_weights_hidden + size_weights_output) * 4",
            /* = Sink vertex            = */  vertex_ForkWeights,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(size_weights_hidden + size_weights_output) * 4");

    /* == Edge ForkWeights[weights_out0] -> [weights]Hidden_layer == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkWeights,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(size_weights_hidden) * 4",
            /* = Sink vertex            = */  vertex_Hidden_layer,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(input_size * hidden_size) * 4");

    /* == Edge ForkWeights[weights_out1] -> [weights]Output_layer == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ForkWeights,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(size_weights_output) * 4",
            /* = Sink vertex            = */  vertex_Output_layer,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(hidden_size * output_size) * 4");

    /* == Return the graph as a Vertex == */
    return spider::api::convertGraphToVertex(graph);
}
