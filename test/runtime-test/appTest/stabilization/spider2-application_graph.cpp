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

#include "spider2-stabilization.h"

/* === Function(s) definition === */

spider::pisdf::Graph *spider::stab::createUserApplicationGraph() {
    /* == Create the graph == */
    auto *graph = spider::api::createGraph(
                             /* = Name of the application graph = */ "top",
                             /* = Number of actors              = */  7,
                             /* = Number of edges               = */  12,
                             /* = Number of parameters          = */  7,
                             /* = Number of input interfaces    = */  0,
                             /* = Number of output interfaces   = */  0,
                             /* = Number of config actors       = */  0);

    /* === Creates the parameter(s) === */

    auto param_width = spider::api::createStaticParam(
          /* = Graph of the parameter = */  graph,
          /* = Name of the parameter  = */ "width",
          /* = Value of the parameter = */  360);

    auto param_height = spider::api::createStaticParam(
          /* = Graph of the parameter = */  graph,
          /* = Name of the parameter  = */ "height",
          /* = Value of the parameter = */  202);

    auto param_id = spider::api::createStaticParam(
          /* = Graph of the parameter = */  graph,
          /* = Name of the parameter  = */ "id",
          /* = Value of the parameter = */  0);

    auto param_border = spider::api::createStaticParam(
          /* = Graph of the parameter = */  graph,
          /* = Name of the parameter  = */ "border",
          /* = Value of the parameter = */  100);

    auto param_displayWidth = spider::api::createDerivedParam(
          /* = Graph of the parameter      = */  graph,
          /* = Name of the parameter       = */ "displayWidth",
          /* = Expression of the parameter = */ "width+2*border");

    auto param_displayHeight = spider::api::createDerivedParam(
          /* = Graph of the parameter      = */  graph,
          /* = Name of the parameter       = */ "displayHeight",
          /* = Expression of the parameter = */ "height+2*border");

    auto param_displaySize = spider::api::createDerivedParam(
          /* = Graph of the parameter      = */  graph,
          /* = Name of the parameter       = */ "displaySize",
          /* = Expression of the parameter = */ "displayHeight*displayWidth");

    /* === Set the input interface(s) === */

    /* === Set the output interface(s) === */

    /* === Creates the actor(s) == */
    
    auto *vertex_DuplicateV = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "DuplicateV", 
          /* = Number of input edges  = */  1, 
          /* = Number of output edges = */  2,
          /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
          /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex DuplicateV == */
    spider::api::addInputParamsToVertex(vertex_DuplicateV, { param_displaySize });

    /* == Setting mappable constraints of the vertex DuplicateV == */
    spider::api::setVertexMappableOnPE(vertex_DuplicateV, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex DuplicateV == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_DuplicateV, TYPE_X86, "100");
    
    auto *vertex_ReadYUV = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "ReadYUV", 
          /* = Number of input edges  = */  0, 
          /* = Number of output edges = */  3,
          /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
          /* = Kernel index (if any)  = */  kernels::READYUV);


    /* == Set the input parameters needed by the refinement of the vertex ReadYUV == */
    spider::api::addInputRefinementParamToVertex(vertex_ReadYUV, param_width);
    spider::api::addInputRefinementParamToVertex(vertex_ReadYUV, param_height);
    
    /* == Setting mappable constraints of the vertex ReadYUV == */
    spider::api::setVertexMappableOnPE(vertex_ReadYUV, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex ReadYUV == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_ReadYUV, TYPE_X86, "100");
    
    auto *vertex_WriteYUV = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "WriteYUV", 
          /* = Number of input edges  = */  3, 
          /* = Number of output edges = */  0,
          /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
          /* = Kernel index (if any)  = */  kernels::YUVWRITE);


    /* == Set the input parameters needed by the refinement of the vertex WriteYUV == */
    spider::api::addInputRefinementParamToVertex(vertex_WriteYUV, param_displayWidth);
    spider::api::addInputRefinementParamToVertex(vertex_WriteYUV, param_displayHeight);
    
    /* == Setting mappable constraints of the vertex WriteYUV == */
    spider::api::setVertexMappableOnPE(vertex_WriteYUV, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex WriteYUV == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_WriteYUV, TYPE_X86, "100");
    
    auto *vertex_DisplayYUV = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "DisplayYUV", 
          /* = Number of input edges  = */  3, 
          /* = Number of output edges = */  0,
          /* = Type of the vertex     = */  pisdf::VertexType::NORMAL,
          /* = Kernel index (if any)  = */  kernels::YUVDISPLAY);


    /* == Set the input parameters used by rate expressions of the vertex DisplayYUV == */
    spider::api::addInputParamsToVertex(vertex_DisplayYUV, { param_displayHeight, param_displayWidth, param_border });

    /* == Set the input parameters needed by the refinement of the vertex DisplayYUV == */
    spider::api::addInputRefinementParamToVertex(vertex_DisplayYUV, param_id);
    
    /* == Setting mappable constraints of the vertex DisplayYUV == */
    spider::api::setVertexMappableOnPE(vertex_DisplayYUV, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex DisplayYUV == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_DisplayYUV, TYPE_X86, "100");
    
    auto *vertex_DuplicateY = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "DuplicateY", 
          /* = Number of input edges  = */  1, 
          /* = Number of output edges = */  2,
          /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
          /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex DuplicateY == */
    spider::api::addInputParamsToVertex(vertex_DuplicateY, { param_displaySize });
    
    /* == Setting mappable constraints of the vertex DuplicateY == */
    spider::api::setVertexMappableOnPE(vertex_DuplicateY, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex DuplicateY == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_DuplicateY, TYPE_X86, "100");
    
    auto *vertex_DuplicateU = spider::api::createVertexFromType(
          /* = Graph of the vertex    = */  graph, 
          /* = Name of the actor      = */ "DuplicateU", 
          /* = Number of input edges  = */  1, 
          /* = Number of output edges = */  2,
          /* = Type of the vertex     = */  pisdf::VertexType::DUPLICATE,
          /* = Kernel index (if any)  = */  SIZE_MAX);


    /* == Set the input parameters used by rate expressions of the vertex DuplicateU == */
    spider::api::addInputParamsToVertex(vertex_DuplicateU, { param_displaySize });
    
    /* == Setting mappable constraints of the vertex DuplicateU == */
    spider::api::setVertexMappableOnPE(vertex_DuplicateU, PE_X86_CORE0, true);
    
    /* == Set the timings of the vertex DuplicateU == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_DuplicateU, TYPE_X86, "100");

    /* === Creates the subgraph(s) === */
    
    auto *vertex_Stabilization = spider::stab::createStabilizationSubgraph("Stabilization", graph, { param_width,  param_height,  param_border });

    /* === Creates the edge(s) === */

    /* == Edge ReadYUV[y] -> [y]Stabilization == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ReadYUV,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(height*width) * 1",
                            /* = Sink vertex            = */  vertex_Stabilization,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(height*width) * 1");   

    /* == Edge ReadYUV[u] -> [u]Stabilization == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ReadYUV,
                            /* = Source port ix         = */  1,
                            /* = Source rate expression = */ "(height/2*width/2) * 1",
                            /* = Sink vertex            = */  vertex_Stabilization,
                            /* = Sink port ix           = */  1,
                            /* = sink rate expression   = */ "(height/2*width/2) * 1");   

    /* == Edge ReadYUV[v] -> [v]Stabilization == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_ReadYUV,
                            /* = Source port ix         = */  2,
                            /* = Source rate expression = */ "(height/2*width/2) * 1",
                            /* = Sink vertex            = */  vertex_Stabilization,
                            /* = Sink port ix           = */  2,
                            /* = sink rate expression   = */ "(height/2*width/2) * 1");   

    /* == Edge Stabilization[rY] -> [in]DuplicateY == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Stabilization,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "((height+2*border)*(width+2*border)) * 1",
                            /* = Sink vertex            = */  vertex_DuplicateY,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(displaySize) * 1");   

    /* == Edge Stabilization[rU] -> [in]DuplicateU == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Stabilization,
                            /* = Source port ix         = */  1,
                            /* = Source rate expression = */ "((height+2*border)/2*(width+2*border)/2) * 1",
                            /* = Sink vertex            = */  vertex_DuplicateU,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(displaySize/4) * 1");   

    /* == Edge Stabilization[rV] -> [in]DuplicateV == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_Stabilization,
                            /* = Source port ix         = */  2,
                            /* = Source rate expression = */ "((height+2*border)/2*(width+2*border)/2) * 1",
                            /* = Sink vertex            = */  vertex_DuplicateV,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(displaySize/4) * 1");   

    /* == Edge DuplicateY[out_0] -> [y]DisplayYUV == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_DuplicateY,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(displaySize) * 1",
                            /* = Sink vertex            = */  vertex_DisplayYUV,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(displayHeight*displayWidth) * 1");   

    /* == Edge DuplicateY[out_1] -> [y]WriteYUV == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_DuplicateY,
                            /* = Source port ix         = */  1,
                            /* = Source rate expression = */ "(displaySize) * 1",
                            /* = Sink vertex            = */  vertex_WriteYUV,
                            /* = Sink port ix           = */  0,
                            /* = sink rate expression   = */ "(displayHeight*displayWidth) * 1");   

    /* == Edge DuplicateU[out_0] -> [u]DisplayYUV == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_DuplicateU,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(displaySize/4) * 1",
                            /* = Sink vertex            = */  vertex_DisplayYUV,
                            /* = Sink port ix           = */  1,
                            /* = sink rate expression   = */ "(displayHeight/2*displayWidth/2) * 1");   

    /* == Edge DuplicateU[out_1] -> [u]WriteYUV == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_DuplicateU,
                            /* = Source port ix         = */  1,
                            /* = Source rate expression = */ "(displaySize/4) * 1",
                            /* = Sink vertex            = */  vertex_WriteYUV,
                            /* = Sink port ix           = */  1,
                            /* = sink rate expression   = */ "(displayWidth/2*displayHeight/2) * 1");   

    /* == Edge DuplicateV[out_0] -> [v]DisplayYUV == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_DuplicateV,
                            /* = Source port ix         = */  0,
                            /* = Source rate expression = */ "(displaySize/4) * 1",
                            /* = Sink vertex            = */  vertex_DisplayYUV,
                            /* = Sink port ix           = */  2,
                            /* = sink rate expression   = */ "(displayHeight/2*displayWidth/2) * 1");   

    /* == Edge DuplicateV[out_1] -> [v]WriteYUV == */
    spider::api::createEdge(/* = Source vertex          = */  vertex_DuplicateV,
                            /* = Source port ix         = */  1,
                            /* = Source rate expression = */ "(displaySize/4) * 1",
                            /* = Sink vertex            = */  vertex_WriteYUV,
                            /* = Sink port ix           = */  2,
                            /* = sink rate expression   = */ "(displayHeight/2*displayWidth/2) * 1");   
    return graph;
}
