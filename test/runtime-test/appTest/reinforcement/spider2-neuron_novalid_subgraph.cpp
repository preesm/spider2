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

spider::pisdf::Vertex *spider::rl::createNeuron_novalidSubgraph(std::string name,
                                                                spider::pisdf::Graph *parentGraph,
                                                                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams) {
    /* == Create the subgraph == */
    auto *graph = spider::api::createSubgraph(parentGraph,
            /* = Name of the subgraph        = */ std::move(name),
            /* = Number of actors            = */ 5,
            /* = Number of edges             = */ 4,
            /* = Number of parameters        = */ 2,
            /* = Number of input interfaces  = */ 3,
            /* = Number of output interfaces = */ 1,
            /* = Number of config actors     = */ 0);

    /* === Creates the parameter(s) === */

    auto param_input_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "input_size",
            /* = Parent parameter       = */  parentGraphParams[0]);

    auto param_layer_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "layer_size",
            /* = Parent parameter       = */  parentGraphParams[1]);

    /* === Set the input interface(s) === */

    auto *vertex_input = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "input");

    auto *vertex_weights = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  1,
            /* = Name of the interface  = */ "weights");

    auto *vertex_bias_values = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  2,
            /* = Name of the interface  = */ "bias_values");

    /* === Set the output interface(s) === */

    auto *vertex_output = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "output");

    /* === Creates the actor(s) == */

    auto *vertex_computeNeuron = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "computeNeuron",
            /* = Number of input edges  = */  3,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::NEURON);


    /* == Set the input parameters needed by the refinement of the vertex computeNeuron == */
    spider::api::addInputRefinementParamToVertex(vertex_computeNeuron, param_input_size);

    /* == Setting mappable constraints of the vertex computeNeuron == */
    spider::api::setVertexMappableOnPE(vertex_computeNeuron, PE_X86_CORE0, true);

    /* == Set the timings of the vertex computeNeuron == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_computeNeuron, TYPE_X86, "100");

    /* === Creates the subgraph(s) === */

    /* === Creates the edge(s) === */

    /* == Edge computeNeuron[output] -> [output]output == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_computeNeuron,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1 * (input_size > 0)) * 4",
            /* = Sink vertex            = */  vertex_output,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(layer_size) * 4");

    /* == Edge bias_values[bias_values] -> [weights]computeNeuron == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_bias_values,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(layer_size) * 4",
            /* = Sink vertex            = */  vertex_computeNeuron,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(1 * (input_size > 0)) * 4");

    /* == Edge input[input] -> [bias_values]computeNeuron == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_input,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(input_size) * 4",
            /* = Sink vertex            = */  vertex_computeNeuron,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(input_size) * 4");

    /* == Edge weights[weights] -> [input]computeNeuron == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_weights,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(input_size * layer_size) * 4",
            /* = Sink vertex            = */  vertex_computeNeuron,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(input_size) * 4");

    /* == Return the graph as a Vertex == */
    return spider::api::convertGraphToVertex(graph);
}
