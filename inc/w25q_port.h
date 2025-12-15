/*
 * w25q_port.h
 *
 *  Created on: Dec 13, 2025
 *      Author: denwoken
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "w25q.h"
#include "w25q_config.h"
#include "w25q_transfer_level.h"


#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_qspi.h"





#define HAL_ERR_TO_W25QSTATUS(halerr,stage) 			 \
(                                                        \
	(halerr) == HAL_TIMEOUT ? W25Q_ERR_TIMEOUT(stage) :  \
	(halerr) == HAL_ERROR   ? W25Q_ERR_ERROR(stage)   :  \
	(halerr) == HAL_BUSY    ? W25Q_ERR_ERROR(stage)   :  \
							  W25Q_UNKNOWN_ERROR         \
)


typedef struct {
    QSPI_HandleTypeDef *hqspi;
} w25q_stm32_port_ctx_t;





/* QSPI/SPI: отправить команду (без данных), используется для простых инструкций */
w25q_status_t w25q_port_send_command(void *port_ctx, const w25q_transfer_t *c, uint32_t Timeout);



w25q_status_t w25q_port_transfer(void *port_ctx, const w25q_transfer_t *c, uint32_t Timeout) ;


w25q_status_t w25q_port_autoPolling(void *port_ctx, const w25q_transfer_t *c, uint8_t wait_msk, uint8_t wait_val, uint32_t Timeout);











