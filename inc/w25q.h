/*
 * w25q.h
 *
 *  Created on: Oct 3, 2025
 *      Author: denwoken
 */

#pragma once
#include <stdbool.h>
#include <stdint.h>
// Write commands
#define W25Q_CMD_WRITE_ENABLE                0x06
#define W25Q_CMD_WRITE_ENABLE_VOLATILE       0x50
#define W25Q_CMD_WRITE_DISABLE               0x04
#define W25Q_CMD_WRITE_STATUS_REGISTER       0x01
#define W25Q_CMD_PAGE_PROGRAM                0x02
#define W25Q_CMD_QUAD_PAGE_PROGRAM           0x32
#define W25Q_CMD_SECTOR_ERASE_4KB            0x20
#define W25Q_CMD_BLOCK_ERASE_32KB            0x52
#define W25Q_CMD_BLOCK_ERASE_64KB            0xD8
#define W25Q_CMD_CHIP_ERASE                  0xC7  // или 0x60
#define W25Q_CMD_ERASE_PROGRAM_SUSPEND       0x75
#define W25Q_CMD_ERASE_PROGRAM_RESUME        0x7A
#define W25Q_CMD_POWER_DOWN                  0xB9
#define W25Q_CMD_CONTINUOUS_READ_MODE        0xFF  // Reset / continuous read mode

// Read commands
#define W25Q_CMD_READ_DATA                   0x03
#define W25Q_CMD_FAST_READ                   0x0B
#define W25Q_CMD_FAST_READ_DUAL_OUTPUT       0x3B
#define W25Q_CMD_FAST_READ_QUAD_OUTPUT       0x6B
#define W25Q_CMD_FAST_READ_DUAL_IO           0xBB
#define W25Q_CMD_FAST_READ_QUAD_IO           0xEB
#define W25Q_CMD_WORD_READ_QUAD_IO           0xE7
#define W25Q_CMD_OCTAL_WORD_READ_QUAD_IO     0xE3
#define W25Q_CMD_SET_BURST_WITH_WRAP         0x77

// Status register commands
#define W25Q_CMD_READ_STATUS_REGISTER_1      0x05
#define W25Q_CMD_READ_STATUS_REGISTER_2      0x35


// Device / ID commands
#define W25Q_CMD_RELEASE_POWER_DOWN_ID       0xAB  // Release Power-down / Device ID
#define W25Q_CMD_MANUFACTURER_DEVICE_ID      0x90  // Manufacturer/Device ID
#define W25Q_CMD_MANUFACTURER_DEVICE_ID_DUAL_IO 0x92  // Manufacturer/Device ID via Dual I/O
#define W25Q_CMD_MANUFACTURER_DEVICE_ID_QUAD_IO 0x94  // Manufacturer/Device ID via Quad I/O
#define W25Q_CMD_JEDEC_ID                     0x9F  // JEDEC ID (Manufacturer, Memory Type, Capacity)
#define W25Q_CMD_READ_UNIQUE_ID               0x4B  // Unique 64-bit ID

// SFDP commands
#define W25Q_CMD_READ_SFDP_REGISTER           0x5A  // Read Serial Flash Discoverable Parameters

// Security Register commands
#define W25Q_CMD_ERASE_SECURITY_REGISTERS     0x44  // Erase Security Registers
#define W25Q_CMD_PROGRAM_SECURITY_REGISTERS   0x42  // Program Security Registers
#define W25Q_CMD_READ_SECURITY_REGISTERS      0x48  // Read Security Registers

#define W25Q_LOW_ADDRESS 					  0x0
#define W25Q_HIGH_ADDRESS 					  0x3FFFFF

#define W25Q_PAGE_SIZE					 	  0x100
#define W25Q_PAGE_SIZE_MSK					  0xFF
#define W25Q_PAGENUM_TO_MEMADDR(pagenum) (pagenum << 8)

#include "stm32f7xx_hal.h"

#define W25Q_BASE_SECTOR_SIZE        4096U

typedef union {
	struct{
		uint8_t BUSY : 1;   // S0  Erase/Write in Progress
		uint8_t WEL  : 1;   // S1  Write Enable Latch
		uint8_t BP0  : 1;   // S2  Block Protect bit 0
		uint8_t BP1  : 1;   // S3  Block Protect bit 1
		uint8_t BP2  : 1;   // S4  Block Protect bit 2
		uint8_t TB   : 1;   // S5  Top/Bottom Protect
		uint8_t SEC  : 1;   // S6  Sector Protect
		uint8_t SRP0 : 1;   // S7  Status Register Protect 0
	};
	uint8_t raw;
} w25q_StatusReg1_t;


typedef union {
	struct {
		uint8_t SRP1 : 1;   // S8  Status Register Protect 1
		uint8_t QE   : 1;   // S9  Quad Enable
		uint8_t R    : 1;   // S10 Reserved
		uint8_t LB1  : 1;   // S11 Security Register Lock 1
		uint8_t LB2  : 1;   // S12 Security Register Lock 2
		uint8_t LB3  : 1;   // S13 Security Register Lock 3
		uint8_t CMP  : 1;   // S14 Complement Protect
		uint8_t SUS  : 1;   // S15 Suspend Status
	};
    uint8_t raw;
} w25q_StatusReg2_t;

typedef struct {
	w25q_StatusReg1_t sreg1;
	w25q_StatusReg2_t sreg2;
}w25q_StatusRegs_t;



typedef struct{
	uint8_t Manufacturer;
	uint8_t MemoryType;
	uint8_t Capacity;

	uint8_t uuid[8];

}w25q_info_t;

typedef struct
{
	QSPI_HandleTypeDef *hqspi;
	w25q_info_t info;
	w25q_StatusRegs_t cachedStatusRegs;
	bool isCachedStatusReg1Valid;
	bool isCachedStatusReg2Valid;

	uint32_t flash_freq;

}w25q_flash_t;
typedef w25q_flash_t* w25q_flash_handle_t;

HAL_StatusTypeDef W25Q_init(w25q_flash_handle_t handle);



HAL_StatusTypeDef W25Q_ReadInfo(w25q_flash_handle_t handle, w25q_info_t* info);



HAL_StatusTypeDef W25Q_ReadID(w25q_flash_handle_t handle, uint8_t *id); // 3bytes
HAL_StatusTypeDef W25Q_ReadUUID(w25q_flash_handle_t handle, uint8_t *id); // 8bytes



HAL_StatusTypeDef W25Q_ReadStatusReg1(w25q_flash_handle_t handle, w25q_StatusReg1_t* reg);
HAL_StatusTypeDef W25Q_ReadStatusReg2(w25q_flash_handle_t handle, w25q_StatusReg2_t* reg);

HAL_StatusTypeDef W25Q_WriteStatusRegs(w25q_flash_handle_t handle, w25q_StatusRegs_t* regs);

HAL_StatusTypeDef W25Q_UpdateSatatus(w25q_flash_handle_t handle);


HAL_StatusTypeDef W25Q_sendCommand(w25q_flash_handle_t handle, uint8_t cmd);

HAL_StatusTypeDef W25Q_writeEnable(w25q_flash_handle_t handle);
HAL_StatusTypeDef W25Q_writeDisable(w25q_flash_handle_t handle);




HAL_StatusTypeDef W25Q_PageProgramm(w25q_flash_handle_t handle, uint32_t adress, uint8_t* data, uint16_t size);

HAL_StatusTypeDef W25Q_ReadData(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size);

HAL_StatusTypeDef W25Q_AutoPollingMemReady(w25q_flash_handle_t handle, uint32_t timeout);


typedef enum {
	W25Q_SECTOR_TYPE_4K,
	W25Q_SECTOR_TYPE_32K, // block
	W25Q_SECTOR_TYPE_64K, // block
	W25Q_SECTOR_TYPE_ALLCHIP
}w25q_sector_t;
HAL_StatusTypeDef W25Q_SectorErase(w25q_flash_handle_t handle, uint32_t adress, w25q_sector_t sector);



// fast functions

HAL_StatusTypeDef W25Q_QuadPageProgramm(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size);




typedef enum {
	W25Q_FR_MODE_SIO,
	W25Q_FR_MODE_DO,
	W25Q_FR_MODE_QO,
	W25Q_FR_MODE_DIO,
	W25Q_FR_MODE_QIO,
	W25Q_FR_MODE_BEST_AVAILABLE
}w25q_fr_mode_t;


HAL_StatusTypeDef W25Q_FastRead(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size, w25q_fr_mode_t mode);













