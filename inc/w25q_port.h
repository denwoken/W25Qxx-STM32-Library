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

#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_qspi.h"

#include "w25q_transfer_level.h"


typedef struct {
    QSPI_HandleTypeDef *hqspi;
    DMA_HandleTypeDef  *hdma;
} w25q_stm32_port_ctx_t;



#define HAL_ERR_TO_W25QSTATUS(halerr,stage) 			 \
(                                                        \
	(halerr) == HAL_TIMEOUT ? W25Q_ERR_TIMEOUT(stage) :  \
	(halerr) == HAL_ERROR   ? W25Q_ERR_ERROR(stage)   :  \
	(halerr) == HAL_BUSY    ? W25Q_ERR_ERROR(stage)   :  \
							  W25Q_UNKNOWN_ERROR         \
)




/* DCache operations (if CPU has D-cache and its enabled) */
//void w25q_port_dcache_clean(void *addr, size_t len);
//void w25q_port_dcache_invalidate(void *addr, size_t len);
//void w25q_port_dcache_clean_unaligned(void *addr, size_t len);
//




/* Инициализация порта (вызвать до использования драйвера) */
//bool w25q_port_init(void){};
//void w25q_port_deinit(void){};
//



/* QSPI/SPI: отправить команду (без данных), используется для простых инструкций */
w25q_status_t w25q_port_send_command(void *port_ctx, const w25q_transfer_t *c, uint32_t Timeout);

//w25q_status_t w25q_port_transmit(void *port_ctx, const w25q_transfer_t *c, uint32_t Timeout);
//w25q_status_t w25q_port_receive(void *port_ctx, const w25q_transfer_t *c, uint32_t Timeout);


w25q_status_t w25q_port_transfer(void *port_ctx, const w25q_transfer_t *c, uint32_t Timeout) ;


w25q_status_t w25q_port_autoPooling(void *port_ctx, const w25q_transfer_t *c, uint8_t wait_msk, uint8_t wait_val, uint32_t Timeout);



/*
w25q_status_t w25q_port_autoPooling_wait(void *ctx, uint8_t cmd,
		uint8_t flag,
		uint32_t Timeout){


	QSPI_CommandTypeDef sCommand;
	memset(&sCommand, 0, sizeof(sCommand));

	sCommand.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction         = W25Q_CMD_READ_STATUS_REGISTER_1;
	sCommand.AddressMode         = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode            = QSPI_DATA_1_LINE;
	sCommand.NbData              = 1;
	sCommand.DummyCycles         = 0;
	sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;


	QSPI_AutoPollingTypeDef sConfig;
	memset(&sConfig, 0, sizeof(sConfig));
	w25q_StatusReg1_t StatusRegMsk = {0};
	StatusRegMsk.BUSY = 1; // looking only busy flag
	sConfig.Mask = StatusRegMsk.raw;
	sConfig.Match = 0; // busy should be zeroed
	sConfig.Interval = 0x10;
	sConfig.MatchMode = QSPI_MATCH_MODE_AND;
	sConfig.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
	sConfig.StatusBytesSize = 1;


	if (HAL_QSPI_AutoPolling(handle->hqspi, &sCommand, &sConfig, timeout) != HAL_OK)
		return HAL_ERROR;

}
*/

/* Передача/приём данных (поллинг) */
//w25q_status_t w25q_port_transmit(void *ctx, uint8_t *buf, size_t len){};
//w25q_status_t w25q_port_receive(void *ctx, uint8_t *buf, size_t len){};


















