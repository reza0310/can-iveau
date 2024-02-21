/*
 * can-iveau.h
 *
 *      Author: Geoffroy Du Prey (geoffroy.du-prey@epita.fr)
 */

#ifndef CAN_H
#define CAN_H

#include <stdbool.h>
#include <stdio.h>  // Debug seulement
#include "stm32f3xx_hal.h"
#include "main.h"
#include "queue.h"

typedef struct __packed canManager_s {
	CAN_HandleTypeDef* hcan;
	uint8_t boardType;
	uint8_t boardId;
	canMessageQueue_t queue;  // J'en ai fait un pointeur car c'est ce que prennent les fonctions de queue en argument.
} canManager_t;

void setReceivingManager(canManager_t* manager);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan);  // Pour chopper l'interrupt. La définition ne pouvant probablement pas être modifiée, veuillez setReceivingManager avant.

HAL_StatusTypeDef caniveauStart(canManager_t* manager, CAN_HandleTypeDef* hcan, uint8_t board_type, uint8_t board_id, uint8_t mailbox_size);  // Pour l'initialisation
HAL_StatusTypeDef caniveauAddFilter(CAN_HandleTypeDef* hcan, uint8_t filter_num, uint32_t filter_id, uint32_t filter_mask);
HAL_StatusTypeDef caniveauGenerateFilters(canManager_t* manager);
HAL_StatusTypeDef caniveauSendRaw(canManager_t* manager, uint32_t header, uint64_t data);
HAL_StatusTypeDef caniveauSendParsed(canManager_t* manager, uint8_t priority, uint8_t message_type, uint8_t message_id, uint8_t board_type, uint8_t board_id, uint8_t tracking, uint64_t data);
HAL_StatusTypeDef caniveauSendParsedChecked(canManager_t* manager, uint8_t priority, uint8_t message_type, uint8_t message_id, uint8_t board_type, uint8_t board_id, uint8_t tracking, uint64_t data);
HAL_StatusTypeDef caniveauReceiveRaw(canManager_t* manager, canData_t* data);  // Pour la lecture
HAL_StatusTypeDef caniveauReceiveParsed(canManager_t* manager, uint8_t* priority, uint8_t* message_type, uint8_t* message_id, uint8_t* board_type, uint8_t* board_id, uint8_t* tracking, uint64_t* data);
HAL_StatusTypeDef caniveauStop(canManager_t* manager);

// ATTENTION MAUVAIS ID ET DATA AU PREMIER MSG APRES RESET
// VERIFIER DATA RECU

#endif // CAN_H
