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

#ifndef SPIDER2_APPLICATION_TRAINING_H
#define SPIDER2_APPLICATION_TRAINING_H

/* === Include(s) === */

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <spider.h>
#include "include/environment.h"
#include "include/critic.h"
#include "include/actor.h"
#include "include/common.h"
#include "include/mlp.h"

namespace spider {

    namespace rl {

        /* === Enumerations declaration === */

        enum HardwareType : uint32_t {
            TYPE_X86,
        };

        enum HardwareID : uint32_t {
            PE_X86_CORE0,
        };

        enum kernels : size_t {
            STEP = 0,
            RENDERENV = 1,
            TD_ERROR = 2,
            ACTIONSAMPLER = 3,
            SIGMAGEN = 4,
            ACTORLEARNINGRATEGEN = 5,
            CRITICLEARNINGRATEGEN = 6,
            CLIPVALUES = 7,
            ENVACTIONLIMITS = 8,
            ACTIVATETANHYPERBOLIC = 9,
            ACTIVATELINEAR = 10,
            NEURON = 11,
            ADAMEPSILONGEN = 12,
            ADAMUPDATEBETAS = 13,
            APPLYADAMOPTIMIZER = 14,
            DERIVATIVETANHYPERBOLIC = 15,
            COMPUTELAYERBACKPROPERROR = 16,
            COMPUTEWEIGHTSGRADIENTS = 17,
            COMPUTEOUTPUTERROR = 18,
            DERIVATIVELINEAR = 19,
            SETNUMBEROFUPDATE = 20,
            ACTORUPDATEITERATOR = 21,
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

        inline spider::pisdf::Graph *createReinforcementLearning() {
            return rl::createUserApplicationGraph();
        }

        /* === Kernel prototype(s) declaration === */

        /**
         * @brief step function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void stepRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief renderEnv function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void renderenvRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *[]);

        /**
         * @brief td_error function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void td_errorRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief actionSampler function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void actionsamplerRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief sigmaGen function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void sigmagenRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]);

        /**
         * @brief actorLearningRateGen function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void actorlearningrategenRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]);

        /**
         * @brief criticLearningRateGen function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void criticlearningrategenRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]);

        /**
         * @brief clipValues function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void clipvaluesRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief envActionLimits function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void envactionlimitsRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]);

        /**
         * @brief activateTanHyperbolic function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void activatetanhyperbolicRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief activateLinear function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void activatelinearRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief neuron function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void neuronRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief adamEpsilonGen function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void adamepsilongenRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]);

        /**
         * @brief adamUpdateBetas function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void adamupdatebetasRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief applyAdamOptimizer function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void applyadamoptimizerRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief derivativeTanHyperbolic function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void derivativetanhyperbolicRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief computeLayerBackPropError function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void
        computelayerbackproperrorRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief computeWeightsGradients function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void computeweightsgradientsRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief computeOutputError function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void computeoutputerrorRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief derivativeLinear function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void derivativelinearRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]);

        /**
         * @brief setNumberOfUpdate function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void setnumberofupdateRTKernel(const int64_t [], int64_t outputParams[], void *inputs[], void *outputs[]);

        /**
         * @brief actorUpdateIterator function.
         * @param inputParams   Const array of input parameters value.
         * @param outputParams  Array of output parameters to be set by the function.
         * @param input         Array of input data buffers.
         * @param outputs       Array of output data buffers.
         */
        void actorupdateiteratorRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]);

        /* === Graph prototype(s) declaration === */

        /**
         * @brief Generates an instance of the Layer_gradients subgraph.
         * @param name              Name of the instance of the Layer_gradients subgraph.
         * @param parentGraph       Pointer to the parent graph.
         * @param parentGraphParams Vector of parameters of the parent graph.
         * @return pointer to the @refitem pisdf::Vertex created.
         */
        pisdf::Vertex *createLayer_gradientsSubgraph(
                std::string name, spider::pisdf::Graph *parentGraph,
                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams = { });

        /**
         * @brief Generates an instance of the Mlp subgraph.
         * @param name              Name of the instance of the Mlp subgraph.
         * @param parentGraph       Pointer to the parent graph.
         * @param parentGraphParams Vector of parameters of the parent graph.
         * @return pointer to the @refitem pisdf::Vertex created.
         */
        pisdf::Vertex *createMlpSubgraph(
                std::string name, spider::pisdf::Graph *parentGraph,
                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams = { });

        /**
         * @brief Generates an instance of the Network_train subgraph.
         * @param name              Name of the instance of the Network_train subgraph.
         * @param parentGraph       Pointer to the parent graph.
         * @param parentGraphParams Vector of parameters of the parent graph.
         * @return pointer to the @refitem pisdf::Vertex created.
         */
        pisdf::Vertex *createNetwork_trainSubgraph(
                std::string name, spider::pisdf::Graph *parentGraph,
                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams = { });

        /**
         * @brief Generates an instance of the Output_gradients subgraph.
         * @param name              Name of the instance of the Output_gradients subgraph.
         * @param parentGraph       Pointer to the parent graph.
         * @param parentGraphParams Vector of parameters of the parent graph.
         * @return pointer to the @refitem pisdf::Vertex created.
         */
        pisdf::Vertex *createOutput_gradientsSubgraph(
                std::string name, spider::pisdf::Graph *parentGraph,
                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams = { });

        /**
         * @brief Generates an instance of the Mlp_raw subgraph.
         * @param name              Name of the instance of the Mlp_raw subgraph.
         * @param parentGraph       Pointer to the parent graph.
         * @param parentGraphParams Vector of parameters of the parent graph.
         * @return pointer to the @refitem pisdf::Vertex created.
         */
        pisdf::Vertex *createMlp_rawSubgraph(
                std::string name, spider::pisdf::Graph *parentGraph,
                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams = { });

        /**
         * @brief Generates an instance of the Neuron_novalid subgraph.
         * @param name              Name of the instance of the Neuron_novalid subgraph.
         * @param parentGraph       Pointer to the parent graph.
         * @param parentGraphParams Vector of parameters of the parent graph.
         * @return pointer to the @refitem pisdf::Vertex created.
         */
        pisdf::Vertex *createNeuron_novalidSubgraph(
                std::string name, spider::pisdf::Graph *parentGraph,
                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams = { });

        /**
         * @brief Generates an instance of the Train_actor subgraph.
         * @param name              Name of the instance of the Train_actor subgraph.
         * @param parentGraph       Pointer to the parent graph.
         * @param parentGraphParams Vector of parameters of the parent graph.
         * @return pointer to the @refitem pisdf::Vertex created.
         */
        pisdf::Vertex *createTrain_actorSubgraph(
                std::string name, spider::pisdf::Graph *parentGraph,
                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams = { });

        /**
         * @brief Generates an instance of the Network_train_iter subgraph.
         * @param name              Name of the instance of the Network_train_iter subgraph.
         * @param parentGraph       Pointer to the parent graph.
         * @param parentGraphParams Vector of parameters of the parent graph.
         * @return pointer to the @refitem pisdf::Vertex created.
         */
        pisdf::Vertex *createNetwork_train_iterSubgraph(
                std::string name, spider::pisdf::Graph *parentGraph,
                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams = { });

        /**
         * @brief Generates an instance of the Adam subgraph.
         * @param name              Name of the instance of the Adam subgraph.
         * @param parentGraph       Pointer to the parent graph.
         * @param parentGraphParams Vector of parameters of the parent graph.
         * @return pointer to the @refitem pisdf::Vertex created.
         */
        pisdf::Vertex *createAdamSubgraph(
                std::string name, spider::pisdf::Graph *parentGraph,
                const std::vector<std::shared_ptr<spider::pisdf::Param>> &parentGraphParams = { });
    }
}
#endif
