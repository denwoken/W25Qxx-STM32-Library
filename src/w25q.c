/*
 * w25q.c
 *
 *  Created on: Oct 3, 2025
 *      Author: denwoken
 */
#include "w25q_registers.h"
#include "w25q.h"
#include <string.h>
#include "w25q_transfer_level.h"
#include "w25q_port.h"


// таблица строк
const char *const w25q_status_str[] =
{
#define X(name) #name,
    W25Q_STATUS_LIST(X)
#undef X
};




w25q_status_t W25Q_init(w25q_flash_handle_t handle)
{
    W25Q_ASSERT(handle);
    W25Q_ASSERT(handle->port_ctx);

    w25q_status_t status;
    uint32_t prev_clk;

    // --- HW init (port layer) ---
    if (!w25q_port_is_initialized_hw(handle->port_ctx)) {
        status = w25q_port_initialize_hw(handle->port_ctx);
        if (status != W25Q_OK) return status;
    }

    // --- Save current clock & switch to safe ---
    prev_clk = W25Q_getCLK(handle);
    handle->flash_freq = prev_clk;
    status = W25Q_setCLK(handle, W25Q_SAFE_INIT_CLK_HZ);
    if (status != W25Q_OK) return status;

    // --- Read JEDEC / device info ---
    status = W25Q_ReadInfo(handle, &handle->info);


    // --- Restore clock regardless of result ---
    W25Q_setCLK(handle, prev_clk);

    if (status != W25Q_OK) return status;

    // --- Validate device ---
    if (handle->info.Manufacturer == 0x00 ||
        handle->info.MemoryType  == 0x00 ||
        handle->info.Capacity    == 0x00)
    {
        return W25Q_UNKNOWN_ERROR;
    }

    const uint32_t *uuid = (const uint32_t *)handle->info.uuid;
    if ((uuid[0] | uuid[1]) == 0)
        return W25Q_UNKNOWN_ERROR;

    handle->initialized = true;

    return W25Q_OK;
}

bool W25Q_isDMAenabled(w25q_flash_handle_t handle){
	W25Q_ASSERT(handle);
	return w25q_port_is_DMA_enabled(handle->port_ctx);
}
uint32_t W25Q_getCLK(w25q_flash_handle_t handle){
	W25Q_ASSERT(handle);
	return w25q_port_getCLK(handle->port_ctx);
}
w25q_status_t W25Q_setCLK(w25q_flash_handle_t handle, uint32_t clk){
	W25Q_ASSERT(handle);
	w25q_status_t status;
	status = w25q_port_setCLK(handle->port_ctx, clk);
	if(status != W25Q_OK) handle->lastError = status;
	return status;
}



w25q_status_t W25Q_ReadInfo(w25q_flash_handle_t handle, w25q_info_t* info){
	W25Q_ASSERT(handle);
	W25Q_ASSERT(info);

	memset(info, 0, sizeof(w25q_info_t));

	uint8_t id[8];
	memset(id, 0, sizeof(id));

	w25q_status_t status = W25Q_ReadID(handle, id);
	if(status != W25Q_OK) return status;

	info->Manufacturer = id[0];
	info->MemoryType = id[1];
	info->Capacity = id[2];
	info->CapacityBytes = (1UL << info->Capacity);

	status = W25Q_ReadUUID(handle, info->uuid);
	return status;
}



w25q_status_t W25Q_ReadID(w25q_flash_handle_t handle, uint8_t *id)
{
	W25Q_ASSERT(handle);
	W25Q_ASSERT(id);

	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_JEDEC_ID;
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_lines = W25Q_XFER_LINES_NONE;
	transfer.data_lines = W25Q_XFER_LINES_1;
	transfer.direction = W25Q_XFER_RX;
	transfer.data_len = 3;
	transfer.buf = id;

#if DMA_ENABLE==1
	transfer.prefer_dma = (3>=DMA_THRESHOLD);
#endif

    w25q_status_t status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }

    /* basic validation */
    if (id[0] == 0x00 || id[0] == 0xFF)
    	status = W25Q_INVALID_DEVICE;

    return status;

}
w25q_status_t W25Q_ReadUUID(w25q_flash_handle_t handle, uint8_t *id){

	W25Q_ASSERT(handle);
	W25Q_ASSERT(id);

	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_READ_UNIQUE_ID;
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_lines = W25Q_XFER_LINES_NONE;
	transfer.data_lines = W25Q_XFER_LINES_1;
	transfer.direction = W25Q_XFER_RX;
	transfer.data_len = 8;
	transfer.buf = id;
	transfer.alt_bits = W25Q_XFER_BITS_32;
	transfer.alt_data = 0;
	transfer.alt_lines = W25Q_XFER_LINES_1;

#if DMA_ENABLE==1
	transfer.prefer_dma = (8>=DMA_THRESHOLD);
#endif

    w25q_status_t status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK)
    	handle->lastError = status;

    return status;
}




w25q_status_t W25Q_ReadStatusReg1(w25q_flash_handle_t handle, w25q_StatusReg1_t* reg){
	W25Q_ASSERT(handle);

	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_READ_STATUS_REGISTER_1;
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_lines = W25Q_XFER_LINES_NONE;
	transfer.data_lines = W25Q_XFER_LINES_1;
	transfer.direction = W25Q_XFER_RX;
	transfer.data_len = 1;
	transfer.buf = &handle->cachedStatusRegs.sreg1;

    w25q_status_t status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }

    if(reg) *reg = handle->cachedStatusRegs.sreg1;
    handle->isCachedStatusReg1Valid = 1;
    return status;

}
w25q_status_t W25Q_ReadStatusReg2(w25q_flash_handle_t handle, w25q_StatusReg2_t* reg){
	W25Q_ASSERT(handle);


	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_READ_STATUS_REGISTER_2;
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_lines = W25Q_XFER_LINES_NONE;
	transfer.data_lines = W25Q_XFER_LINES_1;
	transfer.direction = W25Q_XFER_RX;
	transfer.data_len = 1;
	transfer.buf = &handle->cachedStatusRegs.sreg2;

    w25q_status_t status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }


    if(reg) *reg = handle->cachedStatusRegs.sreg2;
    handle->isCachedStatusReg2Valid = 1;

    return status;
}

w25q_status_t W25Q_WriteStatusRegs(w25q_flash_handle_t handle, w25q_StatusRegs_t* regs){
	W25Q_ASSERT(handle);
	W25Q_ASSERT(regs);

	w25q_status_t status;
	status = W25Q_writeEnable(handle);
	if(status != W25Q_OK) {
		handle->lastError = status;
		return status;
	}


	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_WRITE_STATUS_REGISTER;
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_lines = W25Q_XFER_LINES_NONE;
	transfer.data_lines = W25Q_XFER_LINES_1;
	transfer.direction = W25Q_XFER_TX;
	transfer.data_len = 2; // SR1 + SR2
	transfer.buf = regs;

    status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) handle->lastError = status;
    return status;

}


w25q_status_t W25Q_UpdateSatatus(w25q_flash_handle_t handle){
	W25Q_ASSERT(handle);

	w25q_status_t status;
	status = W25Q_ReadStatusReg1(handle, NULL);
	if(status != W25Q_OK) {
		handle->lastError = status;
		return status;
	}
	status = W25Q_ReadStatusReg2(handle, NULL);
	if(status != W25Q_OK) {
		handle->lastError = status;
		return status;
	}
	return status;
}


w25q_status_t W25Q_sendCommand(w25q_flash_handle_t handle, uint8_t cmd){
	W25Q_ASSERT(handle);
	w25q_transfer_t transfer = {0};
	transfer.instruction = cmd;
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_lines = W25Q_XFER_LINES_NONE;
	transfer.data_lines = W25Q_XFER_LINES_NONE;
	transfer.direction = W25Q_XFER_NONE;

    w25q_status_t status = w25q_port_send_command(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK)
    	handle->lastError = status;
    return status;
}

w25q_status_t W25Q_writeEnable(w25q_flash_handle_t handle){
	return W25Q_sendCommand(handle, W25Q_CMD_WRITE_ENABLE );
}
w25q_status_t W25Q_writeDisable(w25q_flash_handle_t handle){
	return W25Q_sendCommand(handle, W25Q_CMD_WRITE_DISABLE );
}


w25q_status_t W25Q_PageProgram(w25q_flash_handle_t handle, uint32_t adress, uint8_t* data, uint16_t size){
	W25Q_ASSERT(handle);
	W25Q_ASSERT(data);
    //W25Q_ASSERT(size <= W25Q_PAGE_SIZE);
    //W25Q_ASSERT(((adress & W25Q_PAGE_SIZE_MSK) + size) <= W25Q_PAGE_SIZE);

	w25q_status_t status;
	status = W25Q_writeEnable(handle);
	if(status != W25Q_OK) {
		handle->lastError = status;
		return status;
	}

	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_PAGE_PROGRAM;
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_lines = W25Q_XFER_LINES_1;
	transfer.address = adress;
	transfer.addr_bits = W25Q_XFER_BITS_24;
	transfer.data_lines = W25Q_XFER_LINES_1;
	transfer.direction = W25Q_XFER_TX;
	transfer.data_len = size;
	transfer.buf = data;

#if DMA_ENABLE==1
	transfer.prefer_dma = (size>=DMA_THRESHOLD);
#endif
    status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }


    status = W25Q_AutoPollingMemReady(handle, W25Q_COMMON_TIMEOUT_MS);
	return status;

}



w25q_status_t W25Q_ReadData(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size){
	W25Q_ASSERT(handle);
	W25Q_ASSERT(data);
    W25Q_ASSERT(address < W25Q_HIGH_ADDRESS);
    W25Q_ASSERT((address + size) <= W25Q_HIGH_ADDRESS);


	W25Q_ASSERT(handle);
	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_READ_DATA;
	transfer.instr_lines = W25Q_XFER_LINES_1;

	transfer.addr_lines = W25Q_XFER_LINES_1;
	transfer.addr_bits = W25Q_XFER_BITS_24;

	transfer.data_lines = W25Q_XFER_LINES_1;
	transfer.data_len = size;
	transfer.buf = data;
	transfer.direction = W25Q_XFER_RX;

#if DMA_ENABLE==1
	transfer.prefer_dma = (size>=DMA_THRESHOLD);
#endif

    w25q_status_t status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }
    return status;

}




















w25q_status_t W25Q_AutoPollingMemReady(w25q_flash_handle_t handle, uint32_t timeout){

	W25Q_ASSERT(handle);
	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_READ_STATUS_REGISTER_1;
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_lines = W25Q_XFER_LINES_NONE;
	transfer.data_lines = W25Q_XFER_LINES_1;
	transfer.direction = W25Q_XFER_RX;
	transfer.data_len = 1;

	w25q_StatusReg1_t StatusRegMsk = {0};
	StatusRegMsk.BUSY = 1; // looking only busy flag

    w25q_status_t status = w25q_port_autoPolling(handle, &transfer, StatusRegMsk.raw, 0, timeout);
    if(status != W25Q_OK) handle->lastError = status;
    return status;

}

w25q_status_t W25Q_SectorErase(w25q_flash_handle_t handle, uint32_t address, w25q_sector_t sector){
	W25Q_ASSERT(handle);
    W25Q_ASSERT(address < W25Q_HIGH_ADDRESS);


	w25q_status_t status;
	status = W25Q_writeEnable(handle);
	if(status != W25Q_OK) return status;


	W25Q_ASSERT(handle);
	w25q_transfer_t transfer = {0};
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_bits = W25Q_XFER_BITS_24;

	uint32_t timeout = 0;
	switch(sector){
	case W25Q_SECTOR_TYPE_4K:
		transfer.instruction = W25Q_CMD_SECTOR_ERASE_4KB;
		transfer.addr_lines = W25Q_XFER_LINES_1;
		timeout = W25Q_ERASE_4K_TIMEOUT_MS;
		break;
	case W25Q_SECTOR_TYPE_32K:
		transfer.instruction = W25Q_CMD_BLOCK_ERASE_32KB;
		transfer.addr_lines = W25Q_XFER_LINES_1;
		timeout = W25Q_ERASE_32K_TIMEOUT_MS;
		break;
	case W25Q_SECTOR_TYPE_64K:
		transfer.instruction = W25Q_CMD_BLOCK_ERASE_64KB;
		transfer.addr_lines = W25Q_XFER_LINES_1;
		timeout = W25Q_ERASE_64K_TIMEOUT_MS;
		break;
	case W25Q_SECTOR_TYPE_ALLCHIP:
		transfer.instruction = W25Q_CMD_CHIP_ERASE;
		transfer.addr_lines = W25Q_XFER_LINES_NONE;
		timeout = W25Q_ERASE_FULL_TIMEOUT_MS;
		break;
	}


    status = w25q_port_send_command(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }


    status = W25Q_AutoPollingMemReady(handle, timeout);
    return status;
}






// fast functions


w25q_status_t W25Q_QuadPageProgram(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size){

	W25Q_ASSERT(handle);
	W25Q_ASSERT(data);
//    W25Q_ASSERT(size <= W25Q_PAGE_SIZE);
//    W25Q_ASSERT(((adress & W25Q_PAGE_SIZE_MSK) + size) <= W25Q_PAGE_SIZE);

	w25q_status_t status;
	status = W25Q_writeEnable(handle);
	if(status != W25Q_OK) return status;

    if(!handle->isCachedStatusReg2Valid){
    	status = W25Q_UpdateSatatus(handle);
    	if(status != W25Q_OK) return status;
    }
    if(!handle->cachedStatusRegs.sreg2.QE){
    	handle->cachedStatusRegs.sreg2.QE = 1;
    	status = W25Q_WriteStatusRegs(handle, &handle->cachedStatusRegs);
        if(status != W25Q_OK) return status;
    }


	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_QUAD_PAGE_PROGRAM;
	transfer.instr_lines = W25Q_XFER_LINES_1;

	transfer.addr_lines = W25Q_XFER_LINES_1;
	transfer.addr_bits = W25Q_XFER_BITS_24;

	transfer.data_lines = W25Q_XFER_LINES_4;
	transfer.data_len = size;
	transfer.buf = data;
	transfer.direction = W25Q_XFER_TX;

#if DMA_ENABLE==1
	transfer.prefer_dma = (size>=DMA_THRESHOLD);
#endif

    status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }
    status = W25Q_AutoPollingMemReady(handle, W25Q_COMMON_TIMEOUT_MS);
    return status;
}





w25q_status_t W25Q_FastRead(w25q_flash_handle_t handle,
								uint32_t address,
								uint8_t* data,
								uint32_t size,
								w25q_fr_mode_t mode)
{
	W25Q_ASSERT(handle);
	W25Q_ASSERT(data);
//    W25Q_ASSERT(((adress & W25Q_PAGE_SIZE_MSK) + size) <= W25Q_PAGE_SIZE);


    if(mode == W25Q_FR_MODE_BEST_AVAILABLE)
    	mode = W25Q_FR_MODE_QIO;


    w25q_status_t status;
    if(mode == W25Q_FR_MODE_QIO || mode == W25Q_FR_MODE_QO){
        if(!handle->isCachedStatusReg2Valid){
        	status = W25Q_UpdateSatatus(handle);
        	if(status != W25Q_OK) return status;
        }
        if(!handle->cachedStatusRegs.sreg2.QE){
        	handle->cachedStatusRegs.sreg2.QE = 1;
        	status = W25Q_WriteStatusRegs(handle, &handle->cachedStatusRegs);
			if(status != W25Q_OK) return status;
        }
    }


	w25q_transfer_t transfer = {0};
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_bits = W25Q_XFER_BITS_24;
	transfer.data_len = size;
	transfer.buf = data;
	transfer.direction = W25Q_XFER_RX;

#if DMA_ENABLE==1
	transfer.prefer_dma = (size>=DMA_THRESHOLD);
#endif

    switch(mode){
    case W25Q_FR_MODE_DO:
    	transfer.instruction = W25Q_CMD_FAST_READ_DUAL_OUTPUT;
    	transfer.addr_lines = W25Q_XFER_LINES_1;
    	transfer.data_lines	 = W25Q_XFER_LINES_2;
    	transfer.dummy_cycles = 8;
    	break;
    case W25Q_FR_MODE_QO:
    	transfer.instruction = W25Q_CMD_FAST_READ_QUAD_OUTPUT;
    	transfer.addr_lines = W25Q_XFER_LINES_1;
    	transfer.data_lines	 = W25Q_XFER_LINES_4;
    	transfer.dummy_cycles = 8;
    	break;
    case W25Q_FR_MODE_DIO:
    	transfer.instruction = W25Q_CMD_FAST_READ_DUAL_IO;
    	transfer.addr_lines = W25Q_XFER_LINES_2;
    	transfer.data_lines	 = W25Q_XFER_LINES_2;
    	transfer.alt_bits = W25Q_XFER_BITS_8;
    	transfer.alt_lines = W25Q_XFER_LINES_2;
    	transfer.alt_data = 0x00; // Continuous Read Mode disabled
    	break;
    case W25Q_FR_MODE_QIO:
    	transfer.instruction = W25Q_CMD_FAST_READ_QUAD_IO;
    	transfer.addr_lines = W25Q_XFER_LINES_4;
    	transfer.data_lines	 = W25Q_XFER_LINES_4;
    	transfer.dummy_cycles = 4;
    	transfer.alt_bits = W25Q_XFER_BITS_8;
		transfer.alt_lines = W25Q_XFER_LINES_4;
		transfer.alt_data = 0x00; // Continuous Read Mode disabled
    	break;
    case W25Q_FR_MODE_SIO:
    default:
    	transfer.instruction = W25Q_CMD_FAST_READ;
    	transfer.addr_lines = W25Q_XFER_LINES_1;
    	transfer.data_lines	 = W25Q_XFER_LINES_1;
    	transfer.dummy_cycles = 8;
    }


    status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) handle->lastError = status;
    return status;
}







