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


/*
volatile bool qspi_done = false;
void W25Q_DMA_startTransfer(){
	qspi_done = false;
}


HAL_StatusTypeDef W25Q_DMA_waitTransferComplete(uint32_t Timeout){
    uint32_t Tickstart = HAL_GetTick();

    while (!qspi_done)
    {
        if ((HAL_GetTick() - Tickstart) > Timeout)
            return HAL_TIMEOUT;
        __NOP();
    }
    return HAL_OK;
}

void HAL_QSPI_RxCpltCallback(QSPI_HandleTypeDef *hqspi){
	 qspi_done = true;
}
void HAL_QSPI_TxCpltCallback(QSPI_HandleTypeDef *hqspi){
	 qspi_done = true;
}


static inline void dcache_invalidate_by_addr(void *addr, size_t len)
{
    const uint32_t line = 32;
    const uint32_t lineMsk = (line - 1);
    uintptr_t start = (uintptr_t)addr & ~lineMsk;
    size_t  size = ((len + ((uintptr_t)addr & lineMsk) + lineMsk) & ~lineMsk);

    __DSB();
    if(SCB->CCR & SCB_CCR_DC_Msk)
    	SCB_InvalidateDCache_by_Addr((uint32_t*)start, size);
}
static inline void dcache_clean_by_addr(void *addr, size_t len)
{
	const uint32_t line = 32;
	const uint32_t lineMsk = (line - 1);
	uintptr_t start = (uintptr_t)addr & ~lineMsk;
	size_t  size = ((len + ((uintptr_t)addr & lineMsk) + lineMsk) & ~lineMsk);
    __DSB();
    if(SCB->CCR & SCB_CCR_DC_Msk)
    	SCB_CleanDCache_by_Addr((uint32_t*)start, size);
}
static inline void dcache_clean_unaligned_lines_by_addr(void *addr, size_t len)
{
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
	const uint32_t line = 32;
	const uint32_t lineMsk = (line - 1);

	__DSB();
	if(!(SCB->CCR & SCB_CCR_DC_Msk)) return;

    uintptr_t start_line = (uintptr_t)addr & ~lineMsk;
    uintptr_t end_line   = ((uintptr_t)addr + len - 1) & ~lineMsk;

    //если начало массива не выравнено
    if ((uintptr_t)addr & lineMsk)
        SCB->DCCMVAC = start_line;

    //если  конец массива не выравнен
    if (end_line != start_line && (((uintptr_t)addr + len) & lineMsk))
        SCB->DCCMVAC = end_line;

	__DSB();
	__ISB();
#endif
}

*/


w25q_status_t W25Q_init(w25q_flash_handle_t handle){
	assert_param(handle);
	assert_param(handle->hqspi);

	uint32_t qspi_clk = HAL_RCC_GetHCLKFreq();
	uint32_t prescaler = handle->hqspi->Init.ClockPrescaler + 1;
	handle->flash_freq = qspi_clk / prescaler;

	w25q_status_t status = W25Q_ReadInfo(handle, &handle->info);
	if(status != W25Q_OK) return status;

	if(handle->info.Capacity == 0 ) return W25Q_UNKNOWN_ERROR;
	if(handle->info.MemoryType == 0 ) return W25Q_UNKNOWN_ERROR;
	if(handle->info.Manufacturer == 0 ) return W25Q_UNKNOWN_ERROR;
	uint32_t* uuid = (uint32_t*)handle->info.uuid;
	if((uuid[0] | uuid[1]) == 0) return W25Q_UNKNOWN_ERROR;


//	if(handle->hqspi->hdma){
//
//	}



	return status;
}


w25q_status_t W25Q_ReadInfo(w25q_flash_handle_t handle, w25q_info_t* info){
	assert_param(handle);
	assert_param(info);

	memset(info, 0, sizeof(w25q_info_t));

	uint8_t id[8];
	memset(id, 0, sizeof(id));

	w25q_status_t status = W25Q_ReadID(handle, id);
	if(status != W25Q_OK) return status;

	info->Manufacturer = id[0];
	info->MemoryType = id[1];
	info->Capacity = id[2];

	status = W25Q_ReadUUID(handle, info->uuid);
	return status;
}



w25q_status_t W25Q_ReadID(w25q_flash_handle_t handle, uint8_t *id)
{
	assert_param(handle);
	assert_param(id);

	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_JEDEC_ID;
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_lines = W25Q_XFER_LINES_NONE;
	transfer.data_lines = W25Q_XFER_LINES_1;
	transfer.direction = W25Q_XFER_RX;
	transfer.data_len = 3;
	transfer.buf = id;


    w25q_status_t status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }
    return status;

}
w25q_status_t W25Q_ReadUUID(w25q_flash_handle_t handle, uint8_t *id){

	assert_param(handle);
	assert_param(id);

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

    w25q_status_t status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }
    return status;
}




w25q_status_t W25Q_ReadStatusReg1(w25q_flash_handle_t handle, w25q_StatusReg1_t* reg){
	assert_param(handle);

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
	assert_param(handle);


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
	assert_param(handle);
	assert_param(regs);

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
	assert_param(handle);

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
	assert_param(handle);
	w25q_transfer_t transfer = {0};
	transfer.instruction = cmd;
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_lines = W25Q_XFER_NONE;
	transfer.data_lines = W25Q_XFER_NONE;
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


w25q_status_t W25Q_PageProgramm(w25q_flash_handle_t handle, uint32_t adress, uint8_t* data, uint16_t size){
	assert_param(handle);
	assert_param(data);
    //assert_param(size <= W25Q_PAGE_SIZE);
    //assert_param(((adress & W25Q_PAGE_SIZE_MSK) + size) <= W25Q_PAGE_SIZE);

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

    status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }


    status = W25Q_AutoPollingMemReady(handle, W25Q_COMMON_TIMEOUT_MS);
	return status;

}



w25q_status_t W25Q_ReadData(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size){
	assert_param(handle);
	assert_param(data);
    assert_param(address < W25Q_HIGH_ADDRESS);
    assert_param((address + size) <= W25Q_HIGH_ADDRESS);


	assert_param(handle);
	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_READ_DATA;
	transfer.instr_lines = W25Q_XFER_LINES_1;

	transfer.addr_lines = W25Q_XFER_LINES_1;
	transfer.addr_bits = W25Q_XFER_BITS_24;

	transfer.data_lines = W25Q_XFER_LINES_1;
	transfer.data_len = size;
	transfer.buf = data;
	transfer.direction = W25Q_XFER_RX;

    w25q_status_t status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }
    return status;

}




















w25q_status_t W25Q_AutoPollingMemReady(w25q_flash_handle_t handle, uint32_t timeout){

	assert_param(handle);
	w25q_transfer_t transfer = {0};
	transfer.instruction = W25Q_CMD_READ_STATUS_REGISTER_1;
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_lines = W25Q_XFER_NONE;
	transfer.data_lines = W25Q_XFER_LINES_1;
	transfer.direction = W25Q_XFER_RX;
	transfer.data_len = 1;

	w25q_StatusReg1_t StatusRegMsk = {0};
	StatusRegMsk.BUSY = 1; // looking only busy flag

    w25q_status_t status = w25q_port_autoPooling(handle->port_ctx, &transfer, StatusRegMsk.raw, 0, timeout);
    if(status != W25Q_OK) handle->lastError = status;
    return status;

}

w25q_status_t W25Q_SectorErase(w25q_flash_handle_t handle, uint32_t address, w25q_sector_t sector){
	assert_param(handle);
    assert_param(address < W25Q_HIGH_ADDRESS);


	w25q_status_t status;
	status = W25Q_writeEnable(handle);
	if(status != W25Q_OK) return status;


	assert_param(handle);
	w25q_transfer_t transfer = {0};
	transfer.instr_lines = W25Q_XFER_LINES_1;
	transfer.addr_bits = W25Q_XFER_BITS_24;

	uint32_t timeout = 0;
	switch(sector){
	case W25Q_SECTOR_TYPE_4K:
		transfer.instruction = W25Q_CMD_SECTOR_ERASE_4KB;
		transfer.addr_lines = W25Q_XFER_LINES_1;
		timeout = 400;
		break;
	case W25Q_SECTOR_TYPE_32K:
		transfer.instruction = W25Q_CMD_BLOCK_ERASE_32KB;
		transfer.addr_lines = W25Q_XFER_LINES_1;
		timeout = 800;
		break;
	case W25Q_SECTOR_TYPE_64K:
		transfer.instruction = W25Q_CMD_BLOCK_ERASE_64KB;
		transfer.addr_lines = W25Q_XFER_LINES_1;
		timeout = 1000;
		break;
	case W25Q_SECTOR_TYPE_ALLCHIP:
		transfer.instruction = W25Q_CMD_CHIP_ERASE;
		transfer.addr_lines = W25Q_XFER_LINES_NONE;
		timeout = 15000;
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


w25q_status_t W25Q_QuadPageProgramm(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size){

	assert_param(handle);
	assert_param(data);
//    assert_param(size <= W25Q_PAGE_SIZE);
//    assert_param(((adress & W25Q_PAGE_SIZE_MSK) + size) <= W25Q_PAGE_SIZE);

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

    status = w25q_port_transfer(handle->port_ctx, &transfer, W25Q_COMMON_TIMEOUT_MS);
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }
    status = W25Q_AutoPollingMemReady(handle, W25Q_COMMON_TIMEOUT_MS);
    return status;
}





HAL_StatusTypeDef W25Q_FastRead(w25q_flash_handle_t handle,
								uint32_t address,
								uint8_t* data,
								uint32_t size,
								w25q_fr_mode_t mode)
{
	assert_param(handle);
	assert_param(data);
//    assert_param(((adress & W25Q_PAGE_SIZE_MSK) + size) <= W25Q_PAGE_SIZE);


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
    if(status != W25Q_OK) {
    	handle->lastError = status;
    	return status;
    }


}







