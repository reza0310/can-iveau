/*
 * queue.h
 *
 *      Author: Geoffroy Du Prey (geoffroy.du-prey@epita.fr)
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>  // Debug seulement
#include "stm32f3xx_hal.h"

#define QUEUE_MAX 255  // If more than 255 then change the type of start, end, length, maxLength and any of the same type in the functions declarations
#define ADD(val, max) (val = (val + 1) % max)
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct __packed canData_s {
	uint32_t header;
	uint64_t data;
} canData_t;

typedef struct __packed canMessageQueue_s {
	uint8_t start;
	uint8_t end;
	uint8_t length;
	uint8_t maxLength;
	bool shallOverwrite;
	bool hasLostData;
	canData_t* data;
} canMessageQueue_t;

void queueNew(canMessageQueue_t* res, uint8_t queueSize, bool shallOverwrite);
HAL_StatusTypeDef queueAdd(canMessageQueue_t* queue, canData_t element);
HAL_StatusTypeDef queuePop(canMessageQueue_t* queue, canData_t* data);
uint8_t queueLength(canMessageQueue_t* queue);
bool queueHasLostData(canMessageQueue_t* queue);
HAL_StatusTypeDef queueStop(canMessageQueue_t* queue);

#endif // QUEUE_H
