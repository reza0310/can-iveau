/*
 * can-iveau.c
 *
 *      Author: Geoffroy Du Prey (geoffroy.du-prey@epita.fr)
 */

#include "can-iveau.h"

// ---------- START INTERRUPT FUNCTIONS ----------
canManager_t* MANAGER;

void setReceivingManager(canManager_t* manager) {
	MANAGER = manager;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
	CAN_RxHeaderTypeDef RxHeader;
	uint8_t RxRawData[8];
	canData_t Data;


	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxRawData) != HAL_OK) {  // Si ça retourne un statut différent de OK on est dans la merde
		Error_Handler();
	}
	//printf("Received data: %X.%X.%X.%X.%X.%X.%X.%X\r\n", RxRawData[0], RxRawData[1], RxRawData[2], RxRawData[3], RxRawData[4], RxRawData[5], RxRawData[6], RxRawData[7]);
	Data.header = RxHeader.ExtId;

	Data.data = RxRawData[0];
	for (uint8_t i = 1; i < 8; i++) {
		Data.data <<= 8;
		Data.data += RxRawData[i];
	}

	queueAdd(&MANAGER->queue, Data);  // On récupère l'erreur ou on s'en fout ?
}

// ---------- END INTERRUPT FUNCTIONS ----------

// ---------- START PRIVATE FUNCTIONS ----------

HAL_StatusTypeDef caniveauAddFilter(CAN_HandleTypeDef* hcan, uint8_t filter_num, uint32_t filter_id, uint32_t filter_mask) {
	/* Fonction permettant d'ajouter un filtre au can
	 * [IN] : hcan le contrôleur can, filter_num le numéro du filtre, filter_id l'id que l'on veut filtrer et filter_mask le masque appliqué par et logique sur l'id (pour savoir quels bits vérifier)
	 * [OUT] : Le filtre est appliqué puis un status est retourné.
	 * [NOTE] : Cette fonction est globalement un patchwork de lignes du web donc ne pas toucher et ne pas essayer trop fort de tout comprendre XD
	 */

	// Globalement, toutes les parties de ce code ont été trouvées sur le web.
	CAN_FilterTypeDef filter;  // Le filtre n'existe que dans le scope de cette fonction mais on en a plus besoin une fois appliqué donc pas important.
	filter.FilterActivation = ENABLE;  // Dit si on veux activer les filtres ou non

	// Formules magiques trouvées sur le web (https://schulz-m.github.io/2017/03/23/stm32-can-id-filter/ premier commentaire)
	filter.FilterIdHigh = filter_id >> 13 & 0xFFFF; // STID[10:0] & EXTID[17:13]
	filter.FilterIdLow = filter_id << 3 & 0xFFF8; // EXID[12:5] & 3 Reserved bits
	filter.FilterMaskIdHigh = filter_mask >> 13 & 0xFFFF;
	filter.FilterMaskIdLow = filter_mask << 3 & 0xFFF8;

	filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	filter.FilterBank = filter_num;  // Numéro du filtre pour en gérer plusieurs
	filter.FilterMode = CAN_FILTERMODE_IDMASK;  // Mode d'utilisation des ids et masques pour le filtre
	filter.FilterScale = CAN_FILTERSCALE_32BIT;
	HAL_CAN_ConfigFilter(hcan, &filter);  // Application du filtre
	return HAL_OK;
}

HAL_StatusTypeDef caniveauGenerateFilters(canManager_t* manager) {
	/* Fonction permettant de générer et ajouter les filtres au can en fonction du board id et du board type.
	 * [IN] : hcan le contrôleur can, board_type le type de board et board_id l'id de la board.
	 * [OUT] : Les filtres sont appliqués puis un status est retourné.
	 */

	  // On doit capter 3 adresses:
	  uint8_t filter_num  = 0;  // Broadcast sur les types de board
	  uint32_t filter_id   = 0b00000000000000000000000000000;
	  uint32_t filter_mask = 0b00000000000011111000000000000;
	  caniveauAddFilter(manager->hcan, filter_num, filter_id, filter_mask);

	  filter_num  = 1;  // Type spécifique + broadcast sur les ids de board
	  filter_id   = manager->boardType << 12;
	  filter_mask = 0b00000000000011111111110000000;
	  caniveauAddFilter(manager->hcan, filter_num, filter_id, filter_mask);

	  filter_num  = 2;  // Type spécifique + id spécifique
	  filter_id   = (manager->boardType << 12) + (manager->boardId << 7);
	  caniveauAddFilter(manager->hcan, filter_num, filter_id, filter_mask);
	  return HAL_OK;
}

// ---------- END PRIVATE FUNCTIONS ----------

// ---------- START PUBLIC FUNCTIONS ----------
HAL_StatusTypeDef caniveauStart(canManager_t* manager, CAN_HandleTypeDef* hcan, uint8_t board_type, uint8_t board_id, uint8_t mailbox_size) {

	/* Fonction permettant d'initialiser le CAN.
	 * [IN] : hcan le contrôleur can, board_type le type de board et board_id l'id de la board.
	 * [OUT] : Le CAN est lancé, l'interrupt activé et les filtres appliqués puis un status est retourné.
	 */
	HAL_CAN_Start(hcan);
	if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
		return HAL_ERROR;
	}

	manager->hcan = hcan;
	manager->boardType = board_type;
	manager->boardId = board_id;
	caniveauGenerateFilters(manager);
	queueNew(&manager->queue, mailbox_size, true);

	setReceivingManager(manager);

	return HAL_OK;
}

HAL_StatusTypeDef caniveauSendRaw(canManager_t* manager, uint32_t header, uint64_t data) {
	CAN_TxHeaderTypeDef tx_header;
	tx_header.IDE = CAN_ID_EXT;
	tx_header.ExtId = header;
	tx_header.RTR = CAN_RTR_DATA;
	tx_header.DLC = 8;
	uint32_t tx_mailbox;  // Jsp à quoi ca sert ça
	uint8_t tx_data[8];

	for (uint8_t i = 8; i > 0; i--) {
		tx_data[i-1] = data&0xFF;  // On pourrait faire de 7 à 0 plutôt que 8 a 1 mais comme c'est non signé si je fais >= 0 puis -- ça va tenter d'atteindre -1 et ça va planter
		data >>= 8;
	}

	//printf("Sending data: 0x%X.0x%X.0x%X.0x%X.0x%X.0x%X.0x%X.0x%X\r\n", tx_data[0], tx_data[1], tx_data[2], tx_data[3], tx_data[4], tx_data[5], tx_data[6], tx_data[7]);
	HAL_CAN_AddTxMessage(manager->hcan, &tx_header, tx_data, &tx_mailbox);
	return HAL_OK;
}

HAL_StatusTypeDef caniveauSendParsed(canManager_t* manager, uint8_t priority, uint8_t message_type, uint8_t message_id, uint8_t board_type, uint8_t board_id, uint8_t tracking, uint64_t data) {
	uint32_t header = 0;

	//header <<= 2;  Commenté car techniquement inutile mais sert à comprendre
	header += priority;
	header <<= 2;
	header += message_type;
	header <<= 8;
	header += message_id;
	header <<= 5;
	header += board_type;
	header <<= 5;
	header += board_id;
	header <<= 7;
	header += tracking;

	return caniveauSendRaw(manager, header, data);
}

HAL_StatusTypeDef caniveauSendParsedChecked(canManager_t* manager, uint8_t priority, uint8_t message_type, uint8_t message_id, uint8_t board_type, uint8_t board_id, uint8_t tracking, uint64_t data) {
	if (priority > 3) return HAL_ERROR;  // Pas besoin de checker la borne 0 car c'est non signé
	if (message_type > 3) return HAL_ERROR;
	if (message_id > 255) return HAL_ERROR;
	if (board_type > 31) return HAL_ERROR;
	if (board_id > 31) return HAL_ERROR;
	if (tracking > 127) return HAL_ERROR;

	return caniveauSendParsed(manager, priority, message_type, message_id, board_type, board_id, tracking, data);
}

HAL_StatusTypeDef caniveauReceiveRaw(canManager_t* manager, canData_t* data) {
	return queuePop(&manager->queue, data);
}

HAL_StatusTypeDef caniveauReceiveParsed(canManager_t* manager, uint8_t* priority, uint8_t* message_type, uint8_t* message_id, uint8_t* board_type, uint8_t* board_id, uint8_t* tracking, uint64_t* data) {
	canData_t pop_data;
	if (caniveauReceiveRaw(manager, &pop_data) != HAL_OK)
	{
		return HAL_ERROR;
	}
	*tracking = pop_data.header&0x7F;
	pop_data.header >>= 7;
	*board_id = pop_data.header&0x1F;
	pop_data.header >>= 5;
	*board_type = pop_data.header&0x1F;
	pop_data.header >>= 5;
	*message_id = pop_data.header&0xFF;
	pop_data.header >>= 8;
	*message_type = pop_data.header&0x3;
	pop_data.header >>= 2;
	*priority = pop_data.header&0x3;
	*data = pop_data.data;

	return HAL_OK;
}

HAL_StatusTypeDef caniveauStop(canManager_t* manager) {
	return queueStop(&manager->queue);
}

// ---------- START PRIVATE FUNCTIONS ----------
