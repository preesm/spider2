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

spider::pisdf::Vertex *spider::rl::createOutput_gradientsSubgraph(std::string name,
                                                                  spider::pisdf::Graph *parentGraph,
                                                                  const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams) {
    /* == Create the subgraph == */
    auto *graph = spider::api::createSubgraph(parentGraph,
            /* = Name of the subgraph        = */ std::move(name),
            /* = Number of actors            = */ 10,
            /* = Number of edges             = */ 9,
            /* = Number of parameters        = */ 2,
            /* = Number of input interfaces  = */ 4,
            /* = Number of output interfaces = */ 2,
            /* = Number of config actors     = */ 0);

    /* === Creates the parameter(s) === */

    auto param_output_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "output_size",
            /* = Parent parameter       = */  parentGraphParams[0]);

    auto param_input_size = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "input_size",
            /* = Parent parameter       = */  parentGraphParams[1]);

    /* === Set the input interface(s) === */

    auto *vertex_target = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "target");

    auto *vertex_inputs = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  1,
            /* = Name of the interface  = */ "inputs");

    auto *vertex_raw_output = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  2,
            /* = Name of the interface  = */ "raw_output");

    auto *vertex_output = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  3,
            /* = Name of the interface  = */ "output");

    /* === Set the output interface(s) === */

    auto *vertex_weights_gradient = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "weights_gradient");

    auto *vertex_bias_gradient = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  1,
            /* = Name of the interface  = */ "bias_gradient");

    /* === Creates the actor(s) == */

    auto *vertex_Derivative_Function = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "Derivative_Function",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::DERIVATIVELINEAR);


    /* == Setting mappable constraints of the vertex Derivative_Function == */
    spider::api::setVertexMappableOnPE(vertex_Derivative_Function, PE_X86_CORE0, true);

    /* == Set the timings of the vertex Derivative_Function == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_Derivative_Function, TYPE_X86, "100");

    auto *vertex_Output_Error = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "Output_Error",
            /* = Number of input edges  = */  3,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::COMPUTEOUTPUTERROR);


    /* == Set the input parameters needed by the refinement of the vertex Output_Error == */
    spider::api::addInputRefinementParamToVertex(vertex_Output_Error, param_output_size);

    /* == Setting mappable constraints of the vertex Output_Error == */
    spider::api::setVertexMappableOnPE(vertex_Output_Error, PE_X86_CORE0, true);

    /* == Set the timings of the vertex Output_Error == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_Output_Error, TYPE_X86, "100");

    auto *vertex_Gradients = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "Gradients",
            /* = Number of input edges  = */  2,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::COMPUTEWEIGHTSGRADIENTS);


    /* == Set the input parameters needed by the refinement of the vertex Gradients == */
    spider::api::addInputRefinementParamToVertex(vertex_Gradients, param_input_size);
    spider::api::addInputRefinementParamToVertex(vertex_Gradients, param_output_size);

    /* == Setting mappable constraints of the vertex Gradients == */
    spider::api::setVertexMappableOnPE(vertex_Gradients, PE_X86_CORE0, true);

    /* == Set the timings of the vertex Gradients == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_Gradients, TYPE_X86, "100");

    auto *vertex_BroadcastError = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "BroadcastError",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
            /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BroadcastError == */
    spider::api::addInputParamsToVertex(vertex_BroadcastError, { param_output_size });

    /* == Setting mappable constraints of the vertex BroadcastError == */
    spider::api::setVertexMappableOnPE(vertex_BroadcastError, PE_X86_CORE0, true);

    /* == Set the timings of the vertex BroadcastError == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BroadcastError, TYPE_X86, "100");

    /* === Creates the subgraph(s) === */

    /* === Creates the edge(s) === */

    /* == Edge target[target] -> [target]Output_Error == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_target,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_Output_Error,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge Gradients[gradients] -> [weights_gradient]weights_gradient == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Gradients,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "( input_size * output_size) * 4",
            /* = Sink vertex            = */  vertex_weights_gradient,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(input_size  * output_size) * 4");

    /* == Edge Derivative_Function[output] -> [derivative_values]Output_Error == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Derivative_Function,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 4",
            /* = Sink vertex            = */  vertex_Output_Error,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge Output_Error[errors] -> [in]BroadcastError == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Output_Error,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_BroadcastError,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge BroadcastError[out_1] -> [errors]Gradients == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastError,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_Gradients,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge BroadcastError[out_0] -> [bias_gradient]bias_gradient == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BroadcastError,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_bias_gradient,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge inputs[inputs] -> [inputs]Gradients == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_inputs,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(input_size) * 4",
            /* = Sink vertex            = */  vertex_Gradients,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(input_size) * 4");

    /* == Edge output[output] -> [predicted]Output_Error == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_output,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_Output_Error,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(output_size) * 4");

    /* == Edge raw_output[raw_output] -> [input]Derivative_Function == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_raw_output,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(output_size) * 4",
            /* = Sink vertex            = */  vertex_Derivative_Function,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1) * 4");

    /* == Return the graph as a Vertex == */
    return spider::api::convertGraphToVertex(graph);
}
