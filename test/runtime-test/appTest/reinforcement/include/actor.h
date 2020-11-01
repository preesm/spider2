#ifndef ACTOR_MLP_H__
#define ACTOR_MLP_H__

#include "preesm.h"
#include "common.h"
#include <api/spider.h>

using Param = int64_t;

void actorWeightGenInit(int id, OUT float *weights_out, OUT float *bias_out);


/**
 * @brief Samples a random action for each action in the action space
 *        based on a Normal distribution.
 *        Each distribution is parameterized by mu = action_in[i] and
 *        sigma = sigma_in
 *
 * @param size       Number of actions
 * @param sigma_in   Sigma used for the distribution
 * @param action_in  Vector of input actions
 * @param action_out Vector of output actions
 */
void actionSampler(int size,
                   IN const float *sigma_in, IN const float *action_in,
                   OUT float *action_out);

/**
 * @brief Generate 1 if the actor should be trained, 0 else.
 *
 * @param sigma value of sigma.
 * @param valid boolean of activation.
 */
void setNumberOfUpdate(IN const float *delta, IN float *variance, OUT float *updateVariance, Param *N);

/**
 * @brief Constant generator for sigma value of Normal sampler of action.
 *        Value is defined by SIGMA_GAUSSIAN define
 *        WARNING: This is a work aroung the lack of constant in PREESM
 *
 * @param sigma Generate same value of sigma each firing of the actor
 */
void sigmaGen(OUT float *sigma);

/**
 * @brief Constant generator for learning rate of actor's neural network.
 *        Value is defined by ACTOR_LEARNING_RATE define
 *        WARNING: This is a work aroung the lack of constant in PREESM
 *
 * @param learning_rate Generate same value of learning_rate each firing of the actor
 */
void actorLearningRateGen(OUT float *learning_rate);

void actorUpdateIterator(int *out);


/**
 * @brief Saves the network values.
 *
 * @param n_layer             Number of layers of the network.
 * @param size_layer_weights  Vector containing the number of weights in each layer.
 * @param size_layer_bias     Vector containing the number of bias in each layer.
 * @param weights             Weights vector.
 * @param bias                Bias vector.
 */
void saveNetWork(int n_layer,
                 IN const int *size_layer_weights, IN const int *size_layer_bias, IN float **weights, IN float **bias);

#endif
