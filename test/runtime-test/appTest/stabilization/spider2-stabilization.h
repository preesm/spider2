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

#ifndef SPIDER2_APPLICATION_TOP_H
#define SPIDER2_APPLICATION_TOP_H

/* === Include(s) === */

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <spider.h>
#include "stabilization.h"

namespace spider {

    namespace stab {

        /* === Enumerations declaration === */

        enum HardwareType : uint32_t {
            TYPE_X86,
        };

        enum HardwareID : uint32_t {
            PE_X86_CORE0,
        };

        enum kernels : size_t {
            READYUV = 0,
            YUVDISPLAY = 1,
            YUVWRITE = 2,
            FINDDOMINATINGMOTIONVECTOR = 3,
            RENDERFRAME = 4,
            ACCUMULATEMOTION = 5,
            DIVIDEBLOCKS = 6,
            COMPUTEBLOCKMOTIONVECTOR = 7,
        };

        /* === Functions declaration === */

        /**
         * @brief Creates the physical platform.
         */
        void createUserPhysicalPlatform();

        /**
         * @brief Creates all the runtime kernels and register them into the runtime.
         */
        void createUserApplicationKernels();

        /**
         * @brief Creates the main user application graph.
         * @return Created application graph.
         */
        spider::pisdf::Graph *createUserApplicationGraph();

        inline spider::pisdf::Graph *createStabilization() {
            return stab::createUserApplicationGraph();
        }

        /* === Kernel prototype(s) declaration === */

        /**
         * @brief readYUV function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void readyuvRTKernel(const int64_t inputParams[], int64_t [], void *[], void *outputs[]);

        /**
         * @brief yuvDisplay function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void yuvdisplayRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *[]);

        /**
         * @brief yuvWrite function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void yuvwriteRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *[]);

        /**
         * @brief findDominatingMotionVector function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void
        finddominatingmotionvectorRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief renderFrame function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void renderframeRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief accumulateMotion function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void accumulatemotionRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief divideBlocks function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void divideblocksRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief computeBlockMotionVector function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void computeblockmotionvectorRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /* === Graph prototype(s) declaration === */

        /**
         * @brief Generates an instance of the Stabilization subgraph.
         * @param name              Name of the instance of the Stabilization subgraph.
         * @param parentGraph       Pointer to the parent graph.
         * @param parentGraphParams Vector of parameters of the parent graph.
         * @return pointer to the @refitem pisdf::Vertex created.
         */
        pisdf::Vertex *createStabilizationSubgraph(
                std::string name, spider::pisdf::Graph *parentGraph,
                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams = { });

        /**
         * @brief Generates an instance of the ComputeBlockMotion subgraph.
         * @param name              Name of the instance of the ComputeBlockMotion subgraph.
         * @param parentGraph       Pointer to the parent graph.
         * @param parentGraphParams Vector of parameters of the parent graph.
         * @return pointer to the @refitem pisdf::Vertex created.
         */
        pisdf::Vertex *createComputeBlockMotionSubgraph(
                std::string name, spider::pisdf::Graph *parentGraph,
                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams = { });
    }
}
#endif
