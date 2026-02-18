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

/* === Function(s) declaration === */

void spider::rl::createUserApplicationKernels() {
    /* == Register STEP kernel == */
    spider::api::createRuntimeKernel(stepRTKernel);

    /* == Register RENDERENV kernel == */
    spider::api::createRuntimeKernel(renderenvRTKernel);

    /* == Register TD_ERROR kernel == */
    spider::api::createRuntimeKernel(td_errorRTKernel);

    /* == Register ACTIONSAMPLER kernel == */
    spider::api::createRuntimeKernel(actionsamplerRTKernel);

    /* == Register SIGMAGEN kernel == */
    spider::api::createRuntimeKernel(sigmagenRTKernel);

    /* == Register ACTORLEARNINGRATEGEN kernel == */
    spider::api::createRuntimeKernel(actorlearningrategenRTKernel);

    /* == Register CRITICLEARNINGRATEGEN kernel == */
    spider::api::createRuntimeKernel(criticlearningrategenRTKernel);

    /* == Register CLIPVALUES kernel == */
    spider::api::createRuntimeKernel(clipvaluesRTKernel);

    /* == Register ENVACTIONLIMITS kernel == */
    spider::api::createRuntimeKernel(envactionlimitsRTKernel);

    /* == Register ACTIVATETANHYPERBOLIC kernel == */
    spider::api::createRuntimeKernel(activatetanhyperbolicRTKernel);

    /* == Register ACTIVATELINEAR kernel == */
    spider::api::createRuntimeKernel(activatelinearRTKernel);

    /* == Register NEURON kernel == */
    spider::api::createRuntimeKernel(neuronRTKernel);

    /* == Register ADAMEPSILONGEN kernel == */
    spider::api::createRuntimeKernel(adamepsilongenRTKernel);

    /* == Register ADAMUPDATEBETAS kernel == */
    spider::api::createRuntimeKernel(adamupdatebetasRTKernel);

    /* == Register APPLYADAMOPTIMIZER kernel == */
    spider::api::createRuntimeKernel(applyadamoptimizerRTKernel);

    /* == Register DERIVATIVETANHYPERBOLIC kernel == */
    spider::api::createRuntimeKernel(derivativetanhyperbolicRTKernel);

    /* == Register COMPUTELAYERBACKPROPERROR kernel == */
    spider::api::createRuntimeKernel(computelayerbackproperrorRTKernel);

    /* == Register COMPUTEWEIGHTSGRADIENTS kernel == */
    spider::api::createRuntimeKernel(computeweightsgradientsRTKernel);

    /* == Register COMPUTEOUTPUTERROR kernel == */
    spider::api::createRuntimeKernel(computeoutputerrorRTKernel);

    /* == Register DERIVATIVELINEAR kernel == */
    spider::api::createRuntimeKernel(derivativelinearRTKernel);

    /* == Register SETNUMBEROFUPDATE kernel == */
    spider::api::createRuntimeKernel(setnumberofupdateRTKernel);

    /* == Register ACTORUPDATEITERATOR kernel == */
    spider::api::createRuntimeKernel(actorupdateiteratorRTKernel);
}

/* === step === */

void spider::rl::stepRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]) {
    step(
            /* = state_space_size   = */ static_cast<int>(inputParams[0]),
            /* = action_space_size  = */ static_cast<int>(inputParams[1]),
            /* = state_angular_in   = */ reinterpret_cast<float *>(inputs[0]),
            /* = state_angular_out  = */ reinterpret_cast<float *>(outputs[0]),
            /* = input_actions      = */ reinterpret_cast<float *>(inputs[1]),
            /* = state_observation  = */ reinterpret_cast<float *>(outputs[1]),
            /* = reward             = */ reinterpret_cast<float *>(outputs[2]));
}

/* === renderEnv === */

void spider::rl::renderenvRTKernel(const int64_t [], int64_t [], void *[], void *[]) {
    fprintf(stderr, "[reinforcement] display success!\n");
}

/* === td_error === */

void spider::rl::td_errorRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]) {
    td_error(
            /* = reward           = */ reinterpret_cast<float *>(inputs[0]),
            /* = value_state      = */ reinterpret_cast<float *>(inputs[1]),
            /* = value_next_state = */ reinterpret_cast<float *>(inputs[2]),
            /* = target           = */ reinterpret_cast<float *>(outputs[0]),
            /* = delta            = */ reinterpret_cast<float *>(outputs[1]));
}

/* === actionSampler === */

void spider::rl::actionsamplerRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]) {
    actionSampler(
            /* = size       = */ static_cast<int>(inputParams[0]),
            /* = sigma_in   = */ reinterpret_cast<float *>(inputs[0]),
            /* = action_in  = */ reinterpret_cast<const float *>(inputs[1]),
            /* = action_out = */ reinterpret_cast<float *>(outputs[0]));
}

/* === sigmaGen === */

void spider::rl::sigmagenRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]) {
    sigmaGen(
            /* = sigma = */ reinterpret_cast<float *>(outputs[0]));
}

/* === actorLearningRateGen === */

void spider::rl::actorlearningrategenRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]) {
    actorLearningRateGen(
            /* = learning_rate = */ reinterpret_cast<float *>(outputs[0]));
}

/* === criticLearningRateGen === */

void spider::rl::criticlearningrategenRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]) {
    criticLearningRateGen(
            /* = learning_rate = */ reinterpret_cast<float *>(outputs[0]));
}

/* === clipValues === */

void spider::rl::clipvaluesRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]) {
    clipValues(
            /* = size   = */ static_cast<int>(inputParams[0]),
            /* = input  = */ reinterpret_cast<float *>(inputs[0]),
            /* = limits = */ reinterpret_cast<float *>(inputs[1]),
            /* = output = */ reinterpret_cast<float *>(outputs[0]));
}

/* === envActionLimits === */

void spider::rl::envactionlimitsRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]) {
    envActionLimits(
            /* = limits = */ reinterpret_cast<float *>(outputs[0]));
}

/* === activateTanHyperbolic === */

void spider::rl::activatetanhyperbolicRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]) {
    activateTanHyperbolic(
            /* = input  = */ reinterpret_cast<float *>(inputs[0]),
            /* = output = */ reinterpret_cast<float *>(outputs[0]));
}

/* === activateLinear === */

void spider::rl::activatelinearRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]) {
    activateLinear(
            /* = input  = */ reinterpret_cast<float *>(inputs[0]),
            /* = output = */ reinterpret_cast<float *>(outputs[0]));
}

/* === neuron === */

void spider::rl::neuronRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]) {
    neuron(
            /* = input_size  = */ static_cast<int>(inputParams[0]),
            /* = input       = */ reinterpret_cast<float *>(inputs[0]),
            /* = weights     = */ reinterpret_cast<float *>(inputs[1]),
            /* = bias_values = */ reinterpret_cast<float *>(inputs[2]),
            /* = output      = */ reinterpret_cast<float *>(outputs[0]));
}

/* === adamEpsilonGen === */

void spider::rl::adamepsilongenRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]) {
    adamEpsilonGen(
            /* = epsilon = */ reinterpret_cast<double *>(outputs[0]));
}

/* === adamUpdateBetas === */

void spider::rl::adamupdatebetasRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]) {
    adamUpdateBetas(
            /* = betas_in  = */ reinterpret_cast<double *>(inputs[0]),
            /* = betas_out = */ reinterpret_cast<double *>(outputs[0]));
}

/* === applyAdamOptimizer === */

void spider::rl::applyadamoptimizerRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]) {
    applyAdamOptimizer(
            /* = size          = */ static_cast<int>(inputParams[0]),
            /* = learning_rate = */ reinterpret_cast<float *>(inputs[0]),
            /* = betas         = */ reinterpret_cast<double *>(inputs[1]),
            /* = epsilon       = */ reinterpret_cast<double *>(inputs[2]),
            /* = param_in      = */ reinterpret_cast<float *>(inputs[3]),
            /* = fo_moment_in  = */ reinterpret_cast<double *>(inputs[4]),
            /* = so_moment_in  = */ reinterpret_cast<double *>(inputs[5]),
            /* = gradients     = */ reinterpret_cast<float *>(inputs[6]),
            /* = param_out     = */ reinterpret_cast<float *>(outputs[0]),
            /* = fo_moment_out = */ reinterpret_cast<double *>(outputs[1]),
            /* = so_moment_out = */ reinterpret_cast<double *>(outputs[2]));
}

/* === derivativeTanHyperbolic === */

void spider::rl::derivativetanhyperbolicRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]) {
    derivativeTanHyperbolic(
            /* = input  = */ reinterpret_cast<float *>(inputs[0]),
            /* = output = */ reinterpret_cast<float *>(outputs[0]));
}

/* === computeLayerBackPropError === */

void
spider::rl::computelayerbackproperrorRTKernel(const int64_t inputParams[], int64_t [], void *inputs[],
                                              void *outputs[]) {
    computeLayerBackPropError(
            /* = layer_size         = */ static_cast<int>(inputParams[0]),
            /* = next_layer_size    = */ static_cast<int>(inputParams[1]),
            /* = derivative_values  = */ reinterpret_cast<float *>(inputs[0]),
            /* = next_layer_errors  = */ reinterpret_cast<float *>(inputs[1]),
            /* = next_layer_weights = */ reinterpret_cast<float *>(inputs[2]),
            /* = errors             = */ reinterpret_cast<float *>(outputs[0]));
}

/* === computeWeightsGradients === */

void
spider::rl::computeweightsgradientsRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]) {
    computeWeightsGradients(
            /* = input_size = */ static_cast<int>(inputParams[0]),
            /* = layer_size = */ static_cast<int>(inputParams[1]),
            /* = errors     = */ reinterpret_cast<float *>(inputs[0]),
            /* = inputs     = */ reinterpret_cast<float *>(inputs[1]),
            /* = gradients  = */ reinterpret_cast<float *>(outputs[0]));
}

/* === computeOutputError === */

void spider::rl::computeoutputerrorRTKernel(const int64_t inputParams[], int64_t [], void *inputs[], void *outputs[]) {
    computeOutputError(
            /* = output_size       = */ static_cast<int>(inputParams[0]),
            /* = derivative_values = */ reinterpret_cast<float *>(inputs[0]),
            /* = predicted         = */ reinterpret_cast<float *>(inputs[1]),
            /* = target            = */ reinterpret_cast<float *>(inputs[2]),
            /* = errors            = */ reinterpret_cast<float *>(outputs[0]));
}

/* === derivativeLinear === */

void spider::rl::derivativelinearRTKernel(const int64_t [], int64_t [], void *inputs[], void *outputs[]) {
    derivativeLinear(
            /* = input  = */ reinterpret_cast<float *>(inputs[0]),
            /* = output = */ reinterpret_cast<float *>(outputs[0]));
}

/* === setNumberOfUpdate === */

void spider::rl::setnumberofupdateRTKernel(const int64_t [], int64_t outputParams[], void *inputs[], void *outputs[]) {
    setNumberOfUpdate(
            /* = delta          = */ reinterpret_cast<float *>(inputs[0]),
            /* = variance       = */ reinterpret_cast<float *>(inputs[1]),
            /* = updateVariance = */ reinterpret_cast<float *>(outputs[0]),
            /* = N              = */ static_cast<Param *>(&outputParams[0]));
}

/* === actorUpdateIterator === */

void spider::rl::actorupdateiteratorRTKernel(const int64_t [], int64_t [], void *[], void *outputs[]) {
    actorUpdateIterator(
            /* = out = */ reinterpret_cast<int *>(outputs[0]));
}
