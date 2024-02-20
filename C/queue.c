/*
 * queue.c
 *
 *      Author: Geoffroy Du Prey (geoffroy.du-prey@epita.fr)
 */

#include "queue.h"
// ---------- START QUEUE FUNCTIONS ----------

void queueNew(canMessageQueue_t* res, uint8_t queueSize, bool shallOverwrite) {
	res->start = 1;  // end étant non signé et initialisé a 0, le tout premier élément sera à l'id 1.
	res->end = 0;  // Comme on décale avant de add et on pop avant de décaler, on init start à 1.
	res->length = 0;
	queueSize = MIN(MAX(1, queueSize), QUEUE_MAX);
	res->maxLength = queueSize;
	res->shallOverwrite = shallOverwrite;
	res->hasLostData = false;
	res->data = malloc(queueSize * sizeof(canData_t));
}

HAL_StatusTypeDef queueAdd(canMessageQueue_t* queue, canData_t element) {
	if (queue->length == queue->maxLength){  // Cas file pleine
		queue->hasLostData = true;
		if (queue->shallOverwrite) {
			ADD(queue->end, queue->maxLength);
			// queue->end == queue->start
			ADD(queue->start, queue->maxLength);
			queue->data[queue->end] = element;
		} else {
			return HAL_ERROR;
		}
	} else {  // Cas file non-pleine
		ADD(queue->end, queue->maxLength);
		queue->data[queue->end] = element;  // end étant non signé et initialisé a 0, le tout premier élément sera à l'id 1.
		queue->length++;
	}
	return HAL_OK;
}

HAL_StatusTypeDef queuePop(canMessageQueue_t* queue, canData_t* data) {
	if (queue->length == 0)  // Cas file vide
		return HAL_ERROR;
	queue->length--;  // Si la file n'est pas vide on est forcément dans un cas classique
	*data = queue->data[queue->start];
	ADD(queue->start, queue->maxLength);
	return HAL_OK;
}

uint8_t queueLength(canMessageQueue_t* queue) {
	return queue->length;
}

bool queueHasLostData(canMessageQueue_t* queue) {
	return queue->hasLostData;
}

void queueStop(canMessageQueue_t* queue) {
	free(queue->data);
}

// ---------- END QUEUE FUNCTIONS ----------
