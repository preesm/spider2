//
// Created by farresti on 03/11/17.
//

#ifndef CRITIC_MLP_H
#define CRITIC_MLP_H

#include "preesm.h"
#include "common.h"

void criticWeightGenInit(int id, OUT float *weights_out, OUT float *bias_out);

/**
 * @brief Compute the Temporal Difference Error used to update the critic neural network
 *
 * @param gamma_in
 * @param reward
 * @param value_state
 * @param value_next_state Value predicted by the network in current state.
 * @param target           Value of the target for the critic neural network update.
 * @param delta            Value of the TD-error.
 */
void td_error(IN float *reward, IN float *value_state, IN float *value_next_state,
              OUT float *target, OUT float *delta);


/**
 * @brief Constant generator for learning rate of critic's neural network.
 *        Value is defined by CRITIC_LEARNING_RATE define
 *        WARNING: This is a work aroung the lack of constant in PREESM
 *
 * @param learning_rate Generate same value of learning_rate each firing of the actor
 */
void criticLearningRateGen(OUT float *learning_rate);

#endif //CRITIC_MLP_H
