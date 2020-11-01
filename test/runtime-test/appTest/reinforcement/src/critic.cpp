//
// Created by farresti on 03/11/17.
//
// std libs
#include <cstdio>
// file header
#include "../include/critic.h"


void td_error(IN float *reward, IN float *value_state, IN float *value_next_state,
              OUT float *target, OUT float *delta) {
    (*target) = reward[0] + DISCOUNT_FACTOR * value_next_state[0];
    (*delta) = (*target) - value_state[0];
}

void criticLearningRateGen(OUT float *learning_rate) {
    learning_rate[0] = CRITIC_LEARNING_RATE;
}
