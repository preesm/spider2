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

#include "spider2-application.h"

/* === Function(s) definition === */

spider::pisdf::Vertex *spider::createComputeBlockMotionSubgraph(std::string name,
                                                                spider::pisdf::Graph *parentGraph,
                                                                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams) {
    /* == Create the subgraph == */
    auto *graph = spider::api::createSubgraph(parentGraph,
            /* = Name of the subgraph        = */ std::move(name),
            /* = Number of actors            = */ 5,
            /* = Number of edges             = */ 5,
            /* = Number of parameters        = */ 7,
            /* = Number of input interfaces  = */ 2,
            /* = Number of output interfaces = */ 1,
            /* = Number of config actors     = */ 0);

    /* === Creates the parameter(s) === */

    auto param_width = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "width",
            /* = Parent parameter       = */  parentGraphParams[0]);

    auto param_height = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "height",
            /* = Parent parameter       = */  parentGraphParams[1]);

    auto param_blockWidth = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "blockWidth",
            /* = Parent parameter       = */  parentGraphParams[2]);

    auto param_blockHeight = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "blockHeight",
            /* = Parent parameter       = */  parentGraphParams[3]);

    auto param_maxDeltaX = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "maxDeltaX",
            /* = Parent parameter       = */  parentGraphParams[4]);

    auto param_maxDeltaY = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "maxDeltaY",
            /* = Parent parameter       = */  parentGraphParams[5]);

    auto param_nbVectors = spider::api::createInheritedParam(
            /* = Graph of the parameter = */  graph,
            /* = Name of the parameter  = */ "nbVectors",
            /* = Parent parameter       = */  parentGraphParams[6]);

    /* === Set the input interface(s) === */

    auto *vertex_frame = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "frame");

    auto *vertex_previousFrame = spider::api::setInputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  1,
            /* = Name of the interface  = */ "previousFrame");

    /* === Set the output interface(s) === */

    auto *vertex_vectors = spider::api::setOutputInterfaceName(
            /* = Graph of the interface = */  graph,
            /* = Index of the interface = */  0,
            /* = Name of the interface  = */ "vectors");

    /* === Creates the actor(s) == */

    auto *vertex_ComputeBlockMotionVector = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "ComputeBlockMotionVector",
            /* = Number of input edges  = */  3,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::COMPUTEBLOCKMOTIONVECTOR);


    /* == Set the input parameters used by rate expressions of the vertex ComputeBlockMotionVector == */
//    spider::api::addInputParamsToVertex(vertex_ComputeBlockMotionVector, { param_nbVectors });
    spider::api::addInputParamToVertex(vertex_ComputeBlockMotionVector, param_nbVectors);

    /* == Set the input parameters needed by the refinement of the vertex ComputeBlockMotionVector == */
    spider::api::addInputRefinementParamToVertex(vertex_ComputeBlockMotionVector, param_width);
    spider::api::addInputRefinementParamToVertex(vertex_ComputeBlockMotionVector, param_height);
    spider::api::addInputRefinementParamToVertex(vertex_ComputeBlockMotionVector, param_blockWidth);
    spider::api::addInputRefinementParamToVertex(vertex_ComputeBlockMotionVector, param_blockHeight);
    spider::api::addInputRefinementParamToVertex(vertex_ComputeBlockMotionVector, param_maxDeltaX);
    spider::api::addInputRefinementParamToVertex(vertex_ComputeBlockMotionVector, param_maxDeltaY);

    /* == Setting mappable constraints of the vertex ComputeBlockMotionVector == */
    spider::api::setVertexMappableOnPE(vertex_ComputeBlockMotionVector, PE_X86_CORE0, true);

    /* == Set the timings of the vertex ComputeBlockMotionVector == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_ComputeBlockMotionVector, TYPE_X86, "100");

    auto *vertex_DivideBlocks = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "DivideBlocks",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  2,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
            /* = Kernel index (if any)  = */  kernels::DIVIDEBLOCKS);


    /* == Set the input parameters used by rate expressions of the vertex DivideBlocks == */
//    spider::api::addInputParamsToVertex(vertex_DivideBlocks, { param_nbVectors });
    spider::api::addInputParamToVertex(vertex_DivideBlocks, param_nbVectors);

    /* == Set the input parameters needed by the refinement of the vertex DivideBlocks == */
    spider::api::addInputRefinementParamToVertex(vertex_DivideBlocks, param_width);
    spider::api::addInputRefinementParamToVertex(vertex_DivideBlocks, param_height);
    spider::api::addInputRefinementParamToVertex(vertex_DivideBlocks, param_blockWidth);
    spider::api::addInputRefinementParamToVertex(vertex_DivideBlocks, param_blockHeight);

    /* == Setting mappable constraints of the vertex DivideBlocks == */
    spider::api::setVertexMappableOnPE(vertex_DivideBlocks, PE_X86_CORE0, true);

    /* == Set the timings of the vertex DivideBlocks == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_DivideBlocks, TYPE_X86, "100");

    /* === Creates the subgraph(s) === */

    /* === Creates the edge(s) === */

    /* == Edge frame[frame] -> [frame]DivideBlocks == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_frame,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(height*width) * 1",
            /* = Sink vertex            = */  vertex_DivideBlocks,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(height*width) * 1");

    /* == Edge DivideBlocks[blocksCoord] -> [blockCoord]ComputeBlockMotionVector == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_DivideBlocks,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(nbVectors) * 8",
            /* = Sink vertex            = */  vertex_ComputeBlockMotionVector,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(1) * 8");

    /* == Edge DivideBlocks[blocksData] -> [blockData]ComputeBlockMotionVector == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_DivideBlocks,
            /* = Source port ix         = */  1,
            /* = Source rate expression = */ "(nbVectors*blockWidth*blockHeight) * 1",
            /* = Sink vertex            = */  vertex_ComputeBlockMotionVector,
            /* = Sink port ix           = */  1,
            /* = sink rate expression   = */ "(blockWidth*blockHeight) * 1");

    /* == Edge ComputeBlockMotionVector[vector] -> [vectors]vectors == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ComputeBlockMotionVector,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(1) * 8",
            /* = Sink vertex            = */  vertex_vectors,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(nbVectors) * 8");

    /* == Edge previousFrame[previousFrame] -> [previousFrame]ComputeBlockMotionVector == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_previousFrame,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(height*width) * 1",
            /* = Sink vertex            = */  vertex_ComputeBlockMotionVector,
            /* = Sink port ix           = */  2,
            /* = sink rate expression   = */ "(width*height) * 1");

    /* == Return the graph as a Vertex == */
    return spider::api::convertGraphToVertex(graph);
}
