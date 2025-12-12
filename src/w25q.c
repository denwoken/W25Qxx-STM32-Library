/*
 * w25q.c
 *
 *  Created on: Oct 3, 2025
 *      Author: denwoken
 */
#include "w25q.h"
#include <string.h>


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




HAL_StatusTypeDef W25Q_init(w25q_flash_handle_t handle){
	assert_param(handle);
	assert_param(handle->hqspi);

	uint32_t qspi_clk = HAL_RCC_GetHCLKFreq();
	uint32_t prescaler = handle->hqspi->Init.ClockPrescaler + 1;
	handle->flash_freq = qspi_clk / prescaler;

	HAL_StatusTypeDef status = W25Q_ReadInfo(handle, &handle->info);
	if(status != HAL_OK) return status;

	if(handle->info.Capacity == 0 ) return HAL_ERROR;
	if(handle->info.MemoryType == 0 ) return HAL_ERROR;
	if(handle->info.Manufacturer == 0 ) return HAL_ERROR;
	uint32_t* uuid = (uint32_t*)handle->info.uuid;
	if((uuid[0] | uuid[1]) == 0) return HAL_ERROR;


	if(handle->hqspi->hdma){

	}



	return HAL_OK;
}


HAL_StatusTypeDef W25Q_ReadInfo(w25q_flash_handle_t handle, w25q_info_t* info){
	assert_param(handle);
	assert_param(info);

	memset(info, 0, sizeof(w25q_info_t));

	uint8_t id[8];
	memset(id, 0, sizeof(id));

	HAL_StatusTypeDef status = W25Q_ReadID(handle, id);
	if(status != HAL_OK) return status;

	info->Manufacturer = id[0];
	info->MemoryType = id[1];
	info->Capacity = id[2];

	memset(id, 0, sizeof(id));
	status = W25Q_ReadUUID(handle, id);
	if(status != HAL_OK) return status;

	memcpy(info->uuid, id, sizeof(id));

	return HAL_OK;
}



HAL_StatusTypeDef W25Q_ReadID(w25q_flash_handle_t handle, uint8_t *id)
{
	assert_param(handle);
	assert_param(id);

    QSPI_CommandTypeDef sCommand;
    memset(&sCommand, 0, sizeof(sCommand));

    sCommand.InstructionMode     = QSPI_INSTRUCTION_1_LINE;   // JEDEC ID всегда 1-1-1
    sCommand.Instruction         = W25Q_CMD_JEDEC_ID;              // JEDEC ID команда
    sCommand.AddressMode         = QSPI_ADDRESS_NONE;
    sCommand.AddressSize         = QSPI_ADDRESS_24_BITS;      // значение не используется, но лучше выставить
    sCommand.Address             = 0;

    sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.AlternateBytesSize  = QSPI_ALTERNATE_BYTES_8_BITS; // значение по умолчанию
    sCommand.AlternateBytes      = 0;

    sCommand.DataMode            = QSPI_DATA_1_LINE;
    sCommand.NbData              = 3; // Manufacturer, Memory Type, Capacity

    sCommand.DummyCycles         = 0;

    sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;

    sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;


    if (HAL_QSPI_Command(handle->hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    if (HAL_QSPI_Receive(handle->hqspi, id, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}
HAL_StatusTypeDef W25Q_ReadUUID(w25q_flash_handle_t handle, uint8_t *id){

	assert_param(handle);
	assert_param(id);

    QSPI_CommandTypeDef sCommand;
    memset(&sCommand, 0, sizeof(sCommand));

    sCommand.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction         = W25Q_CMD_READ_UNIQUE_ID;

    sCommand.AddressMode         = QSPI_ADDRESS_NONE;

    sCommand.AlternateByteMode 	 = QSPI_ALTERNATE_BYTES_NONE;

    sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_1_LINE;
    sCommand.AlternateBytes 	 = 0;
    sCommand.AlternateBytesSize	 = QSPI_ALTERNATE_BYTES_32_BITS;
    // 4 dummy байта = 32 такта


    sCommand.DataMode            = QSPI_DATA_1_LINE;
    sCommand.NbData              = 8;

    sCommand.DummyCycles         = 0 ;

    sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;


    if (HAL_QSPI_Command(handle->hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    if (HAL_QSPI_Receive(handle->hqspi, id, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}




HAL_StatusTypeDef W25Q_ReadStatusReg1(w25q_flash_handle_t handle, w25q_StatusReg1_t* reg){
	assert_param(handle);

	w25q_StatusReg1_t sr1 = {0};
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


    if (HAL_QSPI_Command(handle->hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    if (HAL_QSPI_Receive(handle->hqspi, &handle->cachedStatusRegs.sreg1, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    if(reg) *reg = handle->cachedStatusRegs.sreg1;
    handle->isCachedStatusReg1Valid = 1;
    return HAL_OK;

}
HAL_StatusTypeDef W25Q_ReadStatusReg2(w25q_flash_handle_t handle, w25q_StatusReg2_t* reg){
	assert_param(handle);


    QSPI_CommandTypeDef sCommand;
    memset(&sCommand, 0, sizeof(sCommand));

    sCommand.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction         = W25Q_CMD_READ_STATUS_REGISTER_2;
    sCommand.AddressMode         = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;

    sCommand.DataMode            = QSPI_DATA_1_LINE;
    sCommand.NbData              = 1;
    sCommand.DummyCycles         = 0;

    sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;

    sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;


    if (HAL_QSPI_Command(handle->hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    if (HAL_QSPI_Receive(handle->hqspi, &handle->cachedStatusRegs.sreg2, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    if(reg) *reg = handle->cachedStatusRegs.sreg2;
    handle->isCachedStatusReg2Valid = 1;

    return HAL_OK;
}

HAL_StatusTypeDef W25Q_WriteStatusRegs(w25q_flash_handle_t handle, w25q_StatusRegs_t* regs){
	assert_param(handle);
	assert_param(regs);

	if (W25Q_writeEnable(handle)) return HAL_ERROR;

    QSPI_CommandTypeDef sCommand;
    memset(&sCommand, 0, sizeof(sCommand));

    sCommand.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction         = W25Q_CMD_WRITE_STATUS_REGISTER;
    sCommand.AddressMode         = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;

    sCommand.DataMode            = QSPI_DATA_1_LINE;
    sCommand.NbData              = 2; // SR1 + SR2
    sCommand.DummyCycles         = 0;

    sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;

    sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;


    if (HAL_QSPI_Command(handle->hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    if(HAL_QSPI_Transmit(handle->hqspi, (uint8_t*)regs, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    	return HAL_ERROR;

    return HAL_OK;

}


HAL_StatusTypeDef W25Q_UpdateSatatus(w25q_flash_handle_t handle){
	assert_param(handle);

	if (W25Q_ReadStatusReg1(handle, NULL) != HAL_OK) {
		return HAL_ERROR;
	}

	if (W25Q_ReadStatusReg2(handle, NULL) != HAL_OK) {
		return HAL_ERROR;
	}
	return HAL_OK;
}


HAL_StatusTypeDef W25Q_sendCommand(w25q_flash_handle_t handle, uint8_t cmd){
	assert_param(handle);


    QSPI_CommandTypeDef sCommand;
    memset(&sCommand, 0, sizeof(sCommand));

    sCommand.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction         = cmd;
    sCommand.AddressMode         = QSPI_ADDRESS_NONE;
    sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;
    sCommand.DataMode            = QSPI_DATA_NONE;
    sCommand.NbData              = 0;
    sCommand.DummyCycles         = 0;

    sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;

    sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;


    if (HAL_QSPI_Command(handle->hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;

}

HAL_StatusTypeDef W25Q_writeEnable(w25q_flash_handle_t handle){
	return W25Q_sendCommand(handle, W25Q_CMD_WRITE_ENABLE );
}
HAL_StatusTypeDef W25Q_writeDisable(w25q_flash_handle_t handle){
	return W25Q_sendCommand(handle, W25Q_CMD_WRITE_DISABLE );
}


HAL_StatusTypeDef W25Q_PageProgramm(w25q_flash_handle_t handle, uint32_t adress, uint8_t* data, uint16_t size){
	assert_param(handle);
	assert_param(data);
    //assert_param(size <= W25Q_PAGE_SIZE);
    //assert_param(((adress & W25Q_PAGE_SIZE_MSK) + size) <= W25Q_PAGE_SIZE);

    if (W25Q_writeEnable(handle)) return HAL_ERROR;

    QSPI_CommandTypeDef sCommand;
    memset(&sCommand, 0, sizeof(sCommand));

    sCommand.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction         = W25Q_CMD_PAGE_PROGRAM;
    sCommand.AddressMode         = QSPI_ADDRESS_1_LINE;
    sCommand.AddressSize		 = QSPI_ADDRESS_24_BITS;
    sCommand.Address			 = adress;

    sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;

    sCommand.DataMode            = QSPI_DATA_1_LINE;
    sCommand.NbData              = size;
    sCommand.DummyCycles         = 0;

    sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;

    sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;


    if (HAL_QSPI_Command(handle->hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    if(!handle->hqspi->hdma){
    	if(HAL_QSPI_Transmit(handle->hqspi, (uint8_t*)data, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    		return HAL_ERROR;
    } else {
    	dcache_clean_by_addr(data, size);
		W25Q_DMA_startTransfer();
		if (HAL_QSPI_Transmit_DMA(handle->hqspi, data) != HAL_OK)
		    return HAL_ERROR;
		if (W25Q_DMA_waitTransferComplete(5000) != HAL_OK)
		    return HAL_ERROR;
    }


    if(W25Q_AutoPollingMemReady(handle, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    	return HAL_ERROR;

    return HAL_OK;
}



HAL_StatusTypeDef W25Q_ReadData(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size){
	assert_param(handle);
	assert_param(data);
    assert_param(adress < W25Q_HIGH_ADDRESS);
    assert_param((adress + size) <= W25Q_HIGH_ADDRESS);

    QSPI_CommandTypeDef sCommand;
    memset(&sCommand, 0, sizeof(sCommand));

    sCommand.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction         = W25Q_CMD_READ_DATA;
    sCommand.AddressMode         = QSPI_ADDRESS_1_LINE;
    sCommand.AddressSize		 = QSPI_ADDRESS_24_BITS;
    sCommand.Address			 = address;
    sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;

    sCommand.DataMode            = QSPI_DATA_1_LINE;
    sCommand.NbData              = size; // size;
    sCommand.DummyCycles         = 0;

    sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;

    sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;


    if (HAL_QSPI_Command(handle->hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
       return HAL_ERROR;


    if(!handle->hqspi->hdma){
		if (HAL_QSPI_Receive(handle->hqspi, data, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return HAL_ERROR;
    } else {
    	dcache_clean_unaligned_lines_by_addr(data, size);
		W25Q_DMA_startTransfer();
		if (HAL_QSPI_Receive_DMA(handle->hqspi, data) != HAL_OK)
		    return HAL_ERROR;
		if (W25Q_DMA_waitTransferComplete(5000) != HAL_OK)
		    return HAL_ERROR;
		dcache_invalidate_by_addr(data, size);
    }

    return HAL_OK;
}




HAL_StatusTypeDef W25Q_AutoPollingMemReady(w25q_flash_handle_t handle, uint32_t timeout){

	assert_param(handle);

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

	return HAL_OK;
}

HAL_StatusTypeDef W25Q_SectorErase(w25q_flash_handle_t handle, uint32_t address, w25q_sector_t sector){
	assert_param(handle);
    assert_param(adress < W25Q_HIGH_ADDRESS);


	if(W25Q_writeEnable(handle)) return HAL_ERROR;

	uint32_t Instruction = 0;
	uint32_t AddressMode = 0;
	uint32_t timeout = 0;
	switch(sector){
	case W25Q_SECTOR_TYPE_4K:
		Instruction = W25Q_CMD_SECTOR_ERASE_4KB;
		AddressMode = QSPI_ADDRESS_1_LINE;
		timeout = 400;
		break;
	case W25Q_SECTOR_TYPE_32K:
		Instruction = W25Q_CMD_BLOCK_ERASE_32KB;
		AddressMode = QSPI_ADDRESS_1_LINE;
		timeout = 800;
		break;
	case W25Q_SECTOR_TYPE_64K:
		Instruction = W25Q_CMD_BLOCK_ERASE_64KB;
		AddressMode = QSPI_ADDRESS_1_LINE;
		timeout = 1000;
		break;
	case W25Q_SECTOR_TYPE_ALLCHIP:
		Instruction = W25Q_CMD_CHIP_ERASE;
		AddressMode = QSPI_ADDRESS_NONE;
		timeout = 15000;
		break;
	}

    QSPI_CommandTypeDef sCommand;
    memset(&sCommand, 0, sizeof(sCommand));

    sCommand.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction         = Instruction;
    sCommand.AddressMode         = AddressMode;
    sCommand.AddressSize		 = QSPI_ADDRESS_24_BITS;
    sCommand.Address			 = address;

    sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;

    sCommand.DataMode            = QSPI_DATA_NONE;
    sCommand.NbData              = 0;
    sCommand.DummyCycles         = 0;

    sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;

    sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;


    if (HAL_QSPI_Command(handle->hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;


	if(W25Q_AutoPollingMemReady(handle, timeout))
		return HAL_ERROR;
	return HAL_OK;
}






// fast functions


HAL_StatusTypeDef W25Q_QuadPageProgramm(w25q_flash_handle_t handle, uint32_t address, uint8_t* data, uint32_t size){

	assert_param(handle);
	assert_param(data);
//    assert_param(size <= W25Q_PAGE_SIZE);
//    assert_param(((adress & W25Q_PAGE_SIZE_MSK) + size) <= W25Q_PAGE_SIZE);

    if (W25Q_writeEnable(handle)) return HAL_ERROR;


    if(!handle->isCachedStatusReg2Valid){
    	W25Q_UpdateSatatus(handle);
    }
    if(!handle->cachedStatusRegs.sreg2.QE){
    	handle->cachedStatusRegs.sreg2.QE = 1;
    	if(W25Q_WriteStatusRegs(handle, &handle->cachedStatusRegs)){
    		return HAL_ERROR;
    	}
    }

    QSPI_CommandTypeDef sCommand;
    memset(&sCommand, 0, sizeof(sCommand));

    sCommand.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    sCommand.Instruction         = W25Q_CMD_QUAD_PAGE_PROGRAM;
    sCommand.AddressMode         = QSPI_ADDRESS_1_LINE;
    sCommand.AddressSize		 = QSPI_ADDRESS_24_BITS;
    sCommand.Address			 = address;

    sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;

    sCommand.DataMode            = QSPI_DATA_4_LINES;
    sCommand.NbData              = size;
    sCommand.DummyCycles         = 0;

    sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;

    sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;


    if (HAL_QSPI_Command(handle->hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return HAL_ERROR;

    if(!handle->hqspi->hdma){

		if(HAL_QSPI_Transmit(handle->hqspi, (uint8_t*)data, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return HAL_ERROR;

	} else {
		dcache_clean_by_addr(data, size);
		W25Q_DMA_startTransfer();
		if (HAL_QSPI_Transmit_DMA(handle->hqspi, data) != HAL_OK)
			return HAL_ERROR;

		if (W25Q_DMA_waitTransferComplete(5000) != HAL_OK)
			return HAL_ERROR;
	}


    if(W25Q_AutoPollingMemReady(handle, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    	return HAL_ERROR;

    return HAL_OK;


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

    if(mode == W25Q_FR_MODE_QIO || mode == W25Q_FR_MODE_QO){
        if(!handle->isCachedStatusReg2Valid){
        	W25Q_UpdateSatatus(handle);
        }
        if(!handle->cachedStatusRegs.sreg2.QE){
        	handle->cachedStatusRegs.sreg2.QE = 1;
        	if(W25Q_WriteStatusRegs(handle, &handle->cachedStatusRegs)){
        		return HAL_ERROR;
        	}
        }
    }


    QSPI_CommandTypeDef sCommand;
    memset(&sCommand, 0, sizeof(sCommand));

    sCommand.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
    sCommand.Address	 = address;
    sCommand.NbData              = size;
    sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;


    switch(mode){
    case W25Q_FR_MODE_DO:
    	sCommand.Instruction = W25Q_CMD_FAST_READ_DUAL_OUTPUT;
    	sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
    	sCommand.DataMode	 = QSPI_DATA_2_LINES;
    	sCommand.DummyCycles = 8;
    	sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;
    	break;
    case W25Q_FR_MODE_QO:
    	sCommand.Instruction = W25Q_CMD_FAST_READ_QUAD_OUTPUT;
    	sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
    	sCommand.DataMode	 = QSPI_DATA_4_LINES;
    	sCommand.DummyCycles = 8;
    	sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;
    	break;
    case W25Q_FR_MODE_DIO:
    	sCommand.Instruction = W25Q_CMD_FAST_READ_DUAL_IO;
    	sCommand.AddressMode = QSPI_ADDRESS_2_LINES;
    	sCommand.DataMode	 = QSPI_DATA_2_LINES;
    	sCommand.DummyCycles = 0;
    	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_2_LINES;
		sCommand.AlternateBytes = 0x00;     // Continuous Read Mode disabled
		sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    	break;
    case W25Q_FR_MODE_QIO:
    	sCommand.Instruction = W25Q_CMD_FAST_READ_QUAD_IO;
    	sCommand.AddressMode = QSPI_ADDRESS_4_LINES;
    	sCommand.DataMode	 = QSPI_DATA_4_LINES;
    	sCommand.DummyCycles = 4;
    	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
		sCommand.AlternateBytes = 0x00;    // Continuous Read Mode disabled
		sCommand.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    	break;
    case W25Q_FR_MODE_SIO:
    default:
    	sCommand.Instruction = W25Q_CMD_FAST_READ;
    	sCommand.AddressMode = QSPI_ADDRESS_1_LINE;
    	sCommand.DataMode	 = QSPI_DATA_1_LINE;
    	sCommand.DummyCycles = 8;
    	sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;
    }

	if (HAL_QSPI_Command(handle->hqspi, &sCommand, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

    if(!handle->hqspi->hdma){

		if (HAL_QSPI_Receive(handle->hqspi, data, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return HAL_ERROR;

    } else {

    	dcache_clean_unaligned_lines_by_addr(data, size);
		W25Q_DMA_startTransfer();
		if (HAL_QSPI_Receive_DMA(handle->hqspi, data) != HAL_OK)
		    return HAL_ERROR;

		if (W25Q_DMA_waitTransferComplete(5000) != HAL_OK)
		    return HAL_ERROR;

		dcache_invalidate_by_addr(data, size);

    }

    return HAL_OK;
}







