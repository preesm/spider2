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

spider::pisdf::Vertex *spider::createStabilizationSubgraph(std::string name, 
                                                          spider::pisdf::Graph *parentGraph,
                                                          const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams) {
    /* == Create the subgraph == */
    auto *graph = spider::api::createSubgraph(parentGraph,
          /* = Name of the subgraph        = */ std::move(name),
          /* = Number of actors            = */ 40,
          /* = Number of edges             = */ 35,
          /* = Number of parameters        = */ 10,
          /* = Number of input interfaces  = */ 3,
          /* = Number of output interfaces = */ 3,
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

    auto param_border = spider::api::createInheritedParam(
          /* = Graph of the parameter = */  graph,
          /* = Name of the parameter  = */ "border",
          /* = Parent parameter       = */  parentGraphParams[2]);

    auto param_blockWidth = spider::api::createStaticParam(
          /* = Graph of the parameter = */  graph,
          /* = Name of the parameter  = */ "blockWidth",
          /* = Value of the parameter = */  32);

    auto param_blockHeight = spider::api::createStaticParam(
          /* = Graph of the parameter = */  graph,
          /* = Name of the parameter  = */ "blockHeight",
          /* = Value of the parameter = */  32);

    auto param_maxDeltaY = spider::api::createStaticParam(
          /* = Graph of the parameter = */  graph,
          /* = Name of the parameter  = */ "maxDeltaY",
          /* = Value of the parameter = */  21);

    auto param_maxDeltaX = spider::api::createStaticParam(
          /* = Graph of the parameter = */  graph,
          /* = Name of the parameter  = */ "maxDeltaX",
          /* = Value of the parameter = */  38);

    auto param_nbVectors = spider::api::createDerivedParam(
          /* = Graph of the parameter      = */  graph,
          /* = Name of the parameter       = */ "nbVectors",
          /* = Expression of the parameter = */ "floor(height/blockHeight)*floor(width/blockWidth)");

    auto param_displayWidth = spider::api::createDerivedParam(
          /* = Graph of the parameter      = */  graph,
          /* = Name of the parameter       = */ "displayWidth",
          /* = Expression of the parameter = */ "width+2*border");

    auto param_displayHeight = spider::api::createDerivedParam(
          /* = Graph of the parameter      = */  graph,
          /* = Name of the parameter       = */ "displayHeight",
          /* = Expression of the parameter = */ "height+2*border");

    /* === Set the input interface(s) === */

    auto *vertex_y = spider::api::setInputInterfaceName(
          /* = Graph of the interface = */  graph,
          /* = Index of the interface = */  0,
          /* = Name of the interface  = */ "y");

    auto *vertex_u = spider::api::setInputInterfaceName(
          /* = Graph of the interface = */  graph,
          /* = Index of the interface = */  1,
          /* = Name of the interface  = */ "u");

    auto *vertex_v = spider::api::setInputInterfaceName(
          /* = Graph of the interface = */  graph,
          /* = Index of the interface = */  2,
          /* = Name of the interface  = */ "v");

    /* === Set the output interface(s) === */

    auto *vertex_rY = spider::api::setOutputInterfaceName(
          /* = Graph of the interface = */  graph,
          /* = Index of the interface = */  0,
          /* = Name of the interface  = */ "rY");

    auto *vertex_rU = spider::api::setOutputInterfaceName(
          /* = Graph of the interface = */  graph,
          /* = Index of the interface = */  1,
          /* = Name of the interface  = */ "rU");

    auto *vertex_rV = spider::api::setOutputInterfaceName(
          /* = Graph of the interface = */  graph,
          /* = Index of the interface = */  2,
          /* = Name of the interface  = */ "rV");

    /* === Creates the actor(s) == */
    
    auto *vertex_BrY = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "BrY", 
          /* = Number of input edges  = */  1, 
          /* = Number of output edges = */  2,
          /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
          /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BrY == */
    spider::api::addInputParamsToVertex(vertex_BrY, { param_displayWidth, param_displayHeight });
    
    /* == Setting mappable constraints of the vertex BrY == */
    spider::api::setVertexMappableOnPE(vertex_BrY, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex BrY == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BrY, TYPE_X86, "100");
    
    auto *vertex_brFilteredMotion = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "brFilteredMotion", 
          /* = Number of input edges  = */  1, 
          /* = Number of output edges = */  2,
          /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
          /* = Kernel index (if any)  = */  SIZE_MAX);

    
    /* == Setting mappable constraints of the vertex brFilteredMotion == */
    spider::api::setVertexMappableOnPE(vertex_brFilteredMotion, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex brFilteredMotion == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_brFilteredMotion, TYPE_X86, "100");
    
    auto *vertex_DuplicateFrame = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "DuplicateFrame", 
          /* = Number of input edges  = */  1, 
          /* = Number of output edges = */  3,
          /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
          /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex DuplicateFrame == */
    spider::api::addInputParamsToVertex(vertex_DuplicateFrame, { param_width, param_height });
    
    /* == Setting mappable constraints of the vertex DuplicateFrame == */
    spider::api::setVertexMappableOnPE(vertex_DuplicateFrame, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex DuplicateFrame == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_DuplicateFrame, TYPE_X86, "100");
    
    auto *vertex_AccumulateMotion = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "AccumulateMotion", 
          /* = Number of input edges  = */  3, 
          /* = Number of output edges = */  2,
          /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
          /* = Kernel index (if any)  = */  kernels::ACCUMULATEMOTION);

    
    /* == Setting mappable constraints of the vertex AccumulateMotion == */
    spider::api::setVertexMappableOnPE(vertex_AccumulateMotion, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex AccumulateMotion == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_AccumulateMotion, TYPE_X86, "100");
    
    auto *vertex_FindDominatingMotion = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "FindDominatingMotion", 
          /* = Number of input edges  = */  1, 
          /* = Number of output edges = */  1,
          /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
          /* = Kernel index (if any)  = */  kernels::FINDDOMINATINGMOTIONVECTOR);


    /* == Set the input parameters used by rate expressions of the vertex FindDominatingMotion == */
    spider::api::addInputParamsToVertex(vertex_FindDominatingMotion, { param_width, param_height });

    /* == Set the input parameters needed by the refinement of the vertex FindDominatingMotion == */
    spider::api::addInputRefinementParamToVertex(vertex_FindDominatingMotion, param_nbVectors);
    
    /* == Setting mappable constraints of the vertex FindDominatingMotion == */
    spider::api::setVertexMappableOnPE(vertex_FindDominatingMotion, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex FindDominatingMotion == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_FindDominatingMotion, TYPE_X86, "100");
    
    auto *vertex_BrU = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "BrU", 
          /* = Number of input edges  = */  1, 
          /* = Number of output edges = */  2,
          /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
          /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BrU == */
    spider::api::addInputParamsToVertex(vertex_BrU, { param_displayWidth, param_displayHeight });
    
    /* == Setting mappable constraints of the vertex BrU == */
    spider::api::setVertexMappableOnPE(vertex_BrU, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex BrU == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BrU, TYPE_X86, "100");
    
    auto *vertex_renderFrame = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "renderFrame", 
          /* = Number of input edges  = */  8, 
          /* = Number of output edges = */  3,
          /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
          /* = Kernel index (if any)  = */  kernels::RENDERFRAME);


    /* == Set the input parameters needed by the refinement of the vertex renderFrame == */
    spider::api::addInputRefinementParamToVertex(vertex_renderFrame, param_width);
    spider::api::addInputRefinementParamToVertex(vertex_renderFrame, param_height);
    spider::api::addInputRefinementParamToVertex(vertex_renderFrame, param_displayWidth);
    spider::api::addInputRefinementParamToVertex(vertex_renderFrame, param_displayHeight);
    
    /* == Setting mappable constraints of the vertex renderFrame == */
    spider::api::setVertexMappableOnPE(vertex_renderFrame, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex renderFrame == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_renderFrame, TYPE_X86, "100");
    
    auto *vertex_DuplicateAccumulatedMotion = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "DuplicateAccumulatedMotion", 
          /* = Number of input edges  = */  1, 
          /* = Number of output edges = */  2,
          /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
          /* = Kernel index (if any)  = */  SIZE_MAX);

    
    /* == Setting mappable constraints of the vertex DuplicateAccumulatedMotion == */
    spider::api::setVertexMappableOnPE(vertex_DuplicateAccumulatedMotion, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex DuplicateAccumulatedMotion == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_DuplicateAccumulatedMotion, TYPE_X86, "100");
    
    auto *vertex_BrV = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "BrV", 
          /* = Number of input edges  = */  1, 
          /* = Number of output edges = */  2,
          /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
          /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex BrV == */
    spider::api::addInputParamsToVertex(vertex_BrV, { param_displayWidth, param_displayHeight });
    
    /* == Setting mappable constraints of the vertex BrV == */
    spider::api::setVertexMappableOnPE(vertex_BrV, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex BrV == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_BrV, TYPE_X86, "100");

    /* === Creates the subgraph(s) === */
    
    auto *vertex_ComputeBlockMotionVectorss = spider::createComputeBlockMotionSubgraph("ComputeBlockMotionVectorss", graph, { param_width,  param_height,  param_blockWidth,  param_blockHeight,  param_maxDeltaX,  param_maxDeltaY,  param_nbVectors });

    /* === Creates the edge(s) === */

    /* == Edge DuplicateFrame[out_1] -> [previousFrame]ComputeBlockMotionVectorss == */
    auto *edge_DuplicateFrame_out_1__ComputeBlockMotionVectorss_previousFrame = spider::api::createEdge(
         /* = Source vertex          = */  vertex_DuplicateFrame,
         /* = Source port ix         = */  1,
         /* = Source rate expression = */ "(height*width) * 1",
         /* = Sink vertex            = */  vertex_ComputeBlockMotionVectorss,
         /* = Sink port ix           = */  1,
         /* = sink rate expression   = */ "(height*width) * 1");
         
    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */  edge_DuplicateFrame_out_1__ComputeBlockMotionVectorss_previousFrame,
                                       /* = Expression of the delay = */ "(height*width) * 1");   

    /* == Edge DuplicateAccumulatedMotion[out_1] -> [accumulatedMotionIn]AccumulateMotion == */
    auto *edge_DuplicateAccumulatedMotion_out_1__AccumulateMotion_accumulatedMotionIn = spider::api::createEdge(
         /* = Source vertex          = */  vertex_DuplicateAccumulatedMotion,
         /* = Source port ix         = */  1,
         /* = Source rate expression = */ "(1) * 8",
         /* = Sink vertex            = */  vertex_AccumulateMotion,
         /* = Sink port ix           = */  1,
         /* = sink rate expression   = */ "(1) * 8");
         
    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */  edge_DuplicateAccumulatedMotion_out_1__AccumulateMotion_accumulatedMotionIn,
                                       /* = Expression of the delay = */ "(1) * 8");   

    /* == Edge BrY[out_1] -> [yPrev]renderFrame == */
    auto *edge_BrY_out_1__renderFrame_yPrev = spider::api::createEdge(
         /* = Source vertex          = */  vertex_BrY,
         /* = Source port ix         = */  1,
         /* = Source rate expression = */ "(displayHeight*displayWidth) * 1",
         /* = Sink vertex            = */  vertex_renderFrame,
         /* = Sink port ix           = */  5,
         /* = sink rate expression   = */ "(displayHeight*displayWidth) * 1");
         
    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */  edge_BrY_out_1__renderFrame_yPrev,
                                       /* = Expression of the delay = */ "(displayHeight*displayWidth) * 1");   

    /* == Edge BrU[out_1] -> [uPrev]renderFrame == */
    auto *edge_BrU_out_1__renderFrame_uPrev = spider::api::createEdge(
         /* = Source vertex          = */  vertex_BrU,
         /* = Source port ix         = */  1,
         /* = Source rate expression = */ "(displayHeight/2*displayWidth/2) * 1",
         /* = Sink vertex            = */  vertex_renderFrame,
         /* = Sink port ix           = */  6,
         /* = sink rate expression   = */ "(displayHeight/2*displayWidth/2) * 1");
         
    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */  edge_BrU_out_1__renderFrame_uPrev,
                                       /* = Expression of the delay = */ "(displayHeight/2*displayWidth/2) * 1");   

    /* == Edge BrV[out_1] -> [vPrev]renderFrame == */
    auto *edge_BrV_out_1__renderFrame_vPrev = spider::api::createEdge(
         /* = Source vertex          = */  vertex_BrV,
         /* = Source port ix         = */  1,
         /* = Source rate expression = */ "(displayHeight/2*displayWidth/2) * 1",
         /* = Sink vertex            = */  vertex_renderFrame,
         /* = Sink port ix           = */  7,
         /* = sink rate expression   = */ "(displayHeight/2*displayWidth/2) * 1");
         
    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */  edge_BrV_out_1__renderFrame_vPrev,
                                       /* = Expression of the delay = */ "(displayHeight/2*displayWidth/2) * 1");   

    /* == Edge brFilteredMotion[out_1] -> [filteredMotionIn]AccumulateMotion == */
    auto *edge_brFilteredMotion_out_1__AccumulateMotion_filteredMotionIn = spider::api::createEdge(
         /* = Source vertex          = */  vertex_brFilteredMotion,
         /* = Source port ix         = */  1,
         /* = Source rate expression = */ "(1) * 8",
         /* = Sink vertex            = */  vertex_AccumulateMotion,
         /* = Sink port ix           = */  2,
         /* = sink rate expression   = */ "(1) * 8");
         
    /* == Set the delay on the edge == */
    spider::api::createPersistentDelay(/* = Edge of the delay       = */  edge_brFilteredMotion_out_1__AccumulateMotion_filteredMotionIn,
                                       /* = Expression of the delay = */ "(1) * 8");   

    /* == Edge DuplicateFrame[out_0] -> [frame]ComputeBlockMotionVectorss == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_DuplicateFrame,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(height*width) * 1",
                            /* = Sink vertex            = */  vertex_ComputeBlockMotionVectorss,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(height*width) * 1");   

    /* == Edge u[u] -> [uIn]renderFrame == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_u,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(height/2*width/2) * 1",
                            /* = Sink vertex            = */  vertex_renderFrame,
                            /* = Sink port ix           = */  3,
                            /* = sink rate expression   = */ "(height/2*width/2) * 1");   

    /* == Edge v[v] -> [vIn]renderFrame == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_v,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(height/2*width/2) * 1",
                            /* = Sink vertex            = */  vertex_renderFrame,
                            /* = Sink port ix           = */  4,
                            /* = sink rate expression   = */ "(height/2*width/2) * 1");   

    /* == Edge ComputeBlockMotionVectorss[vectors] -> [vectors]FindDominatingMotion == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ComputeBlockMotionVectorss,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(nbVectors) * 8",
                            /* = Sink vertex            = */  vertex_FindDominatingMotion,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(nbVectors) * 8");   

    /* == Edge y[y] -> [in]DuplicateFrame == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_y,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(height*width) * 1",
                            /* = Sink vertex            = */  vertex_DuplicateFrame,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(height*width) * 1");   

    /* == Edge BrY[out_0] -> [rY]rY == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BrY,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(displayHeight*displayWidth) * 1",
                            /* = Sink vertex            = */  vertex_rY,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(displayHeight*displayWidth) * 1");   

    /* == Edge DuplicateFrame[out_2] -> [yIn]renderFrame == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_DuplicateFrame,
                            /* = Source port ix         = */  2,
                            /* = Source rate expression = */ "(height*width) * 1",
                            /* = Sink vertex            = */  vertex_renderFrame,
                            /* = Sink port ix           = */  2,
                            /* = sink rate expression   = */ "(height*width) * 1");   

    /* == Edge FindDominatingMotion[dominatingVector] -> [motionVector]AccumulateMotion == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_FindDominatingMotion,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(1) * 8",
                            /* = Sink vertex            = */  vertex_AccumulateMotion,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(1) * 8");   

    /* == Edge BrU[out_0] -> [rU]rU == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BrU,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(displayHeight/2*displayWidth/2) * 1",
                            /* = Sink vertex            = */  vertex_rU,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(displayHeight/2*displayWidth/2) * 1");   

    /* == Edge BrV[out_0] -> [rV]rV == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_BrV,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(displayHeight/2*displayWidth/2) * 1",
                            /* = Sink vertex            = */  vertex_rV,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(displayHeight/2*displayWidth/2) * 1");   

    /* == Edge DuplicateAccumulatedMotion[out_0] -> [delta]renderFrame == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_DuplicateAccumulatedMotion,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(1) * 8",
                            /* = Sink vertex            = */  vertex_renderFrame,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(1) * 8");   

    /* == Edge AccumulateMotion[filteredMotionOut] -> [in]DuplicateAccumulatedMotion == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_AccumulateMotion,
                            /* = Source port ix         = */  1,
                            /* = Source rate expression = */ "(1) * 8",
                            /* = Sink vertex            = */  vertex_DuplicateAccumulatedMotion,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(1) * 8");   

    /* == Edge renderFrame[yOut] -> [in]BrY == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_renderFrame,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(displayHeight*displayWidth) * 1",
                            /* = Sink vertex            = */  vertex_BrY,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(displayHeight*displayWidth) * 1");   

    /* == Edge renderFrame[uOut] -> [in]BrU == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_renderFrame,
                            /* = Source port ix         = */  1,
                            /* = Source rate expression = */ "(displayHeight/2*displayWidth/2) * 1",
                            /* = Sink vertex            = */  vertex_BrU,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(displayHeight/2*displayWidth/2) * 1");   

    /* == Edge renderFrame[vOut] -> [in]BrV == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_renderFrame,
                            /* = Source port ix         = */  2,
                            /* = Source rate expression = */ "(displayHeight/2*displayWidth/2) * 1",
                            /* = Sink vertex            = */  vertex_BrV,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(displayHeight/2*displayWidth/2) * 1");   

    /* == Edge AccumulateMotion[accumulatedMotionOut] -> [in]brFilteredMotion == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_AccumulateMotion,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(1) * 8",
                            /* = Sink vertex            = */  vertex_brFilteredMotion,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(1) * 8");   

    /* == Edge brFilteredMotion[out_0] -> [deltaPrev]renderFrame == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_brFilteredMotion,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(1) * 8",
                            /* = Sink vertex            = */  vertex_renderFrame,
                            /* = Sink port ix           = */  1,
                            /* = sink rate expression   = */ "(1) * 8");   

    /* == Return the graph as a Vertex == */
    return spider::api::convertGraphToVertex(graph);
}
