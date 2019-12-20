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
#ifndef SPIDER2_SCENARIO_API_H
#define SPIDER2_SCENARIO_API_H

/* === Includes === */

#include <cstdint>
#include <string>
#include <api/global-api.h>

namespace spider {

    /* === Function(s) prototype === */

    namespace api {

        /* === Mapping and Timing related API === */

        /**
         * @brief Sets mappable property of a vertex on a given Cluster of processing element.
         * @param vertex  Pointer to the vertex.
         * @param cluster Const pointer to the cluster.
         * @param value   Value to be set (true = mappable, false = non mappable).
         * @throws spider::Exception if vertex is nullptr.
         */
        void setVertexMappableOnCluster(pisdf::ExecVertex *vertex, const Cluster *cluster, bool value = true);

        /**
         * @brief Sets mappable property of a vertex on a given Cluster of processing element.
         * @param vertex    Pointer to the vertex.
         * @param clusterIx Index of the cluster.
         * @param value     Value to be set (true = mappable, false = non mappable).
         * @throws spider::Exception if vertex is nullptr.
         */
        void setVertexMappableOnCluster(pisdf::ExecVertex *vertex, uint32_t clusterIx, bool value = true);

        /**
         * @brief Sets mappable property of a vertex on a given processing element.
         * @param vertex  Pointer to the vertex.
         * @param pe      Const pointer to the processing element.
         * @param value   Value to be set (true = mappable, false = non mappable).
         * @throws spider::Exception if vertex is nullptr.
         */
        void setVertexMappableOnPE(pisdf::ExecVertex *vertex, const PE *pe, bool value = true);

        /**
         * @brief Sets mappable property of a vertex for all processing elements.
         * @param vertex  Pointer to the vertex.
         * @param value   Value to be set (true = mappable, false = non mappable).
         * @throws spider::Exception if vertex is nullptr.
         */
        void setVertexMappableOnAllPE(pisdf::ExecVertex *vertex, bool value = true);

        /**
         * @brief Sets the execution time expression of a vertex on a given processing element.
         * @param vertex            Pointer to the vertex.
         * @param pe                Const pointer to the processing element.
         * @param timingExpression  Expression of the execution time (can be parameterized).
         * @throws spider::Exception if vertex is nullptr.
         */
        void
        setVertexExecutionTimingOnPE(pisdf::ExecVertex *vertex, const PE *pe, std::string timingExpression = "100");

        /**
         * @brief Sets the execution time value of a vertex on a given processing element.
         * @param vertex  Pointer to the vertex.
         * @param pe      Const pointer to the processing element.
         * @param timing  Value of the timing to be set.
         * @throws spider::Exception if vertex is nullptr.
         */
        void setVertexExecutionTimingOnPE(pisdf::ExecVertex *vertex, const PE *pe, int64_t timing = 100);

        /**
         * @brief Sets the execution time expression of a vertex for all processing elements.
         * @param vertex            Pointer to the vertex.
         * @param timingExpression  Expression of the execution time (can be parameterized).
         * @throws spider::Exception if vertex is nullptr.
         */
        void setVertexExecutionTimingOnAllPE(pisdf::ExecVertex *vertex, std::string timingExpression);

        /**
         * @brief Sets the execution time value of a vertex for all processing elements.
         * @param vertex  Pointer to the vertex.
         * @param timing  Value of the timing to be set.
         * @throws spider::Exception if vertex is nullptr.
         */
        void setVertexExecutionTimingOnAllPE(pisdf::ExecVertex *vertex, int64_t timing = 100);

        /* === Runtime kernel related API === */

        /**
         * @brief Creates a new runtime @refitem RTKernel for a given @refitem pisdf::ExecVertex.
         * @param vertex            Pointer to the vertex to associate the kernel to.
         * @param kernel            Kernel function to set.
         * @param inputParamCount   Number of input parameters (can be modified).
         * @param outputParamCount  Number of output parameters (can NOT be modified).
         * @return pointer to the created @refitem RTKernel.
         * @throws spider::Exception if the vertex is nullptr or if the vertex already has a kernel.
         */
        RTKernel *createKernel(pisdf::ExecVertex *vertex,
                               rtkernel kernel,
                               size_t inputParamCount = 0,
                               size_t outputParamCount = 0);

        /**
         * @brief Adds an input parameter to a given @refitem RTKernel.
         * @param kernel     Pointer to the kernel.
         * @param parameter  Pointer to the parameter to add.
         * @throws spider::Exception if either kernel or parameter is nullptr.
         */
        void addRuntimeKernelInputParameter(RTKernel *kernel, pisdf::Param *parameter);

        /**
         * @brief Adds an input parameter to a given @refitem RTKernel.
         * @param kernel     Pointer to the kernel.
         * @param parameter  Pointer to the parameter to add.
         * @throws spider::Exception if either kernel or parameter is nullptr.
         */
        void addRuntimeKernelInputParameter(RTKernel *kernel, pisdf::DynamicParam *parameter);

        /**
         * @brief Adds an input parameter to a given @refitem RTKernel.
         * @param kernel     Pointer to the kernel.
         * @param parameter  Pointer to the parameter to add.
         * @throws spider::Exception if either kernel or parameter is nullptr.
         */
        void addRuntimeKernelInputParameter(RTKernel *kernel, pisdf::InHeritedParam *parameter);

        /**
         * @brief Adds an output parameter to a given @refitem RTKernel.
         * @param kernel     Pointer to the kernel.
         * @param parameter  Pointer to the parameter to add.
         * @throws spider::Exception if either kernel or parameter is nullptr or if all the output parameters have
         * already been set.
         */
        void addRuntimeKernelOutputParameter(RTKernel *kernel, pisdf::DynamicParam *parameter);
    }
}

#endif //SPIDER2_SCENARIO_H
