/*
	============================================================================
	Name        : preesm.h
	Author      : kdesnos
	Version     : 1.0
	Copyright   : CECILL-C, IETR, INSA Rennes
	Description : Usefull declarations for all headers used in preesm.
	============================================================================
*/

#ifndef PREESM_H
#define PREESM_H

#define IN
#define OUT

#include "environment.h"
#include "mlp.h"
#include "actor.h"
#include "critic.h"
#include "common.h"


//TODO:: proper init
#define init_train() { \
envInit((float*)FIFO_Head_broadcastState_end__0, (float*)FIFO_Head_BroadcastStateFeat__1); \
memcpy(FIFO_Head_BroadcastStateFeat__2, FIFO_Head_BroadcastStateFeat__1, 3 * sizeof(float)); \
memcpy(FIFO_Head_BroadcastStateFeat__3, FIFO_Head_BroadcastStateFeat__1, 3 * sizeof(float)); \
memcpy(FIFO_Head_BroadcastStateFeat__0, FIFO_Head_BroadcastStateFeat__1, 3 * sizeof(float)); \
initAdam((float*)FIFO_Head_Update_Actor_Broad__0); \
initAdam((float*)FIFO_Head_Update_Critic_Broa__0); \
randomVectorInitializer(80, 0.f, 1.f, (float*)FIFO_Head_BroadcastWeightsAc__0); \
randomVectorInitializer(20, 0.f, 0.5f, (float*)FIFO_Head_BroadcastWeightsAc__0 + 60 * sizeof(float)); \
constantVectorInitializer(21, 0.f, (float*)FIFO_Head_BroadcastBiasActor__0); \
constantVectorInitializer(21, 0.f, (float*)FIFO_Head_BroadcastBiasCriti__0); \
randomVectorInitializer(80, 0.f, 1.f, (float*)FIFO_Head_BroadcastWeightsCr__0); \
randomVectorInitializer(20, 0.f, 0.5f, (float*)FIFO_Head_BroadcastWeightsCr__0 + 60 * sizeof(float)); \
memcpy(FIFO_Head_BroadcastBiasCriti__1, FIFO_Head_BroadcastBiasCriti__0, 21 * sizeof(float)); \
memcpy(FIFO_Head_BroadcastWeightsCr__1, FIFO_Head_BroadcastWeightsCr__0, 80 * sizeof(float)); \
memcpy(FIFO_Head_BroadcastBiasCriti__2, FIFO_Head_BroadcastBiasCriti__0, 21 * sizeof(float)); \
memcpy(FIFO_Head_BroadcastWeightsCr__2, FIFO_Head_BroadcastWeightsCr__0, 80 * sizeof(float)); \
memcpy(FIFO_Head_BroadcastBiasActor__1, FIFO_Head_BroadcastBiasActor__0, 21 * sizeof(float)); \
memcpy(FIFO_Head_BroadcastWeightsAc__1, FIFO_Head_BroadcastWeightsAc__0, 80 * sizeof(float)); \
}

#define init_pred() {\
envInit((float*)FIFO_Head_broadcastState_end__0, (float*)FIFO_Head_Environment_end_st__0);\
actorWeightGenInit(0, (float*)FIFO_Head_Actor_MLP_action_p__0, (float*)FIFO_Head_Actor_MLP_action_p__1);\
actorWeightGenInit(1, (float*)FIFO_Head_Actor_MLP_action_p__2, (float*)FIFO_Head_Actor_MLP_action_p__3); \
}

typedef unsigned char uchar;

#endif
