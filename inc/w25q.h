/*
 * w25q.h
 *
 *  Created on: Oct 3, 2025
 *      Author: denwoken
 */

#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "w25q_registers.h"
#include "w25q_config.h"


#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_qspi.h"





#define W25Q_STATUS_LIST(X)                  \
    X(W25Q_OK)                               \
    X(W25Q_READ_ERROR)                       \
    X(W25Q_WRITE_ERROR)                      \
    X(W25Q_SEND_COMMAND_ERROR)               \
    X(W25Q_TRANSMIT_DATA_ERROR)              \
    X(W25Q_RECEIVE_DATA_ERROR)               \
    X(W25Q_POOLING_ERROR)                    \
    X(W25Q_TIMEOUT)                          \
    X(W25Q_READ_TIMEOUT)                     \
    X(W25Q_WRITE_TIMEOUT)                    \
    X(W25Q_SEND_COMMAND_TIMEOUT)             \
    X(W25Q_TRANSMIT_DATA_TIMEOUT)            \
    X(W25Q_RECEIVE_DATA_TIMEOUT)             \
    X(W25Q_POOLING_TIMEOUT)                  \
    X(W25Q_DMA_TIMEOUT)						 \
	X(W25Q_UNKNOWN_ERROR)


typedef enum
{
#define X(name) name,
    W25Q_STATUS_LIST(X)
#undef X
} w25q_status_t;

#define W25Q_STATUS_TO_STR(s) \
    ((s) < W25Q_UNKNOWN_ERROR + 1 ? w25q_status_str[s] : "W25Q_INVALID_STATUS")

// таблица строк
static const char *const w25q_status_str[] =
{
#define X(name) #name,
    W25Q_STATUS_LIST(X)
#undef X
};


//
//typedef enum
//{
//  W25Q_OK,
//
//  W25Q_UNKNOWN_ERROR,
//  W25Q_READ_ERROR,
//  W25Q_WRITE_ERROR,
//  W25Q_SEND_COMMAND_ERROR,
//  W25Q_TRANSMIT_DATA_ERROR,
//  W25Q_RECEIVE_DATA_ERROR,
//  W25Q_POOLING_ERROR,
//
//  W25Q_TIMEOUT,
//  W25Q_READ_TIMEOUT,
//  W25Q_WRITE_TIMEOUT,
//  W25Q_SEND_COMMAND_TIMEOUT,
//  W25Q_TRANSMIT_DATA_TIMEOUT,
//  W25Q_RECEIVE_DATA_TIMEOUT,
//  W25Q_POOLING_TIMEOUT,
//
//  W25Q_DMA_TIMEOUT,
//} w25q_status_t;
#define W25Q_ERR_TIMEOUT(stage) (W25Q_##stage##_TIMEOUT)
#define W25Q_ERR_ERROR(stage)   (W25Q_##stage##_ERROR)



typedef struct{
	uint8_t Manufacturer;
	uint8_t MemoryType;
	uint8_t Capacity;

	uint8_t uuid[8];

}w25q_info_t;

typedef struct
{
	QSPI_HandleTypeDef *hqspi;
	void *port_ctx;
	w25q_status_t lastError;
	w25q_info_t info;
	w25q_StatusRegs_t cachedStatusRegs;
	bool isCachedStatusReg1Valid;
	bool isCachedStatusReg2Valid;

	uint32_t flash_freq;

}w25q_flash_t;
typedef w25q_flash_t* w25q_flash_handle_t;

w25q_status_t W25Q_init(w25q_flash_handle_t handle);



w25q_status_t W25Q_ReadInfo(w25q_flash_handle_t handle, w25q_info_t* info);



w25q_status_t W25Q_ReadID(w25q_flash_handle_t handle, uint8_t *id); // 3bytes
w25q_status_t W25Q_ReadUUID(w25q_flash_handle_t handle, uint8_t *id); // 8bytes



w25q_status_t W25Q_ReadStatusReg1(w25q_flash_handle_t handle, w25q_StatusReg1_t* reg);
w25q_status_t W25Q_ReadStatusReg2(w25q_flash_handle_t handle, w25q_StatusReg2_t* reg);

w25q_status_t W25Q_WriteStatusRegs(w25q_flash_handle_t handle, w25q_StatusRegs_t* regs);

w25q_status_t W25Q_UpdateSatatus(w25q_flash_handle_t handle);


w25q_status_t W25Q_sendCommand(w25q_flash_handle_t handle, uint8_t cmd);

w25q_status_t W25Q_writeEnable(w25q_flash_handle_t handle);
w25q_status_t W25Q_writeDisable(w25q_flash_handle_t handle);




w25q_status_t W25Q_PageProgramm(w25q_flash_handle_t handle, uint32_t adress, uint8_t* data, uint16_t size);

w25q_status_t W25Q_ReadData(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size);

w25q_status_t W25Q_AutoPollingMemReady(w25q_flash_handle_t handle, uint32_t timeout);


typedef enum {
	W25Q_SECTOR_TYPE_4K,
	W25Q_SECTOR_TYPE_32K, // block
	W25Q_SECTOR_TYPE_64K, // block
	W25Q_SECTOR_TYPE_ALLCHIP
}w25q_sector_t;
w25q_status_t W25Q_SectorErase(w25q_flash_handle_t handle, uint32_t adress, w25q_sector_t sector);



// fast functions

w25q_status_t W25Q_QuadPageProgramm(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size);




typedef enum {
	W25Q_FR_MODE_SIO,
	W25Q_FR_MODE_DO,
	W25Q_FR_MODE_QO,
	W25Q_FR_MODE_DIO,
	W25Q_FR_MODE_QIO,
	W25Q_FR_MODE_BEST_AVAILABLE
}w25q_fr_mode_t;
HAL_StatusTypeDef W25Q_FastRead(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size, w25q_fr_mode_t mode);













