/*
 * w25q_port.c
 *
 *  Created on: Dec 13, 2025
 *      Author: denwoken
 */

#include <string.h>
#include "w25q_port.h"
#include "w25q_transfer_level.h"





#if USE_STM32_PORT_F7 == 1
// cache specific functions; works only when Data cache enabled
void __dcache_clean_by_addr(void *addr, size_t len){
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
	const uint32_t line = 32;
	const uint32_t lineMsk = (line - 1);
	uintptr_t start = (uintptr_t)addr & ~lineMsk;
	size_t  size = ((len + ((uintptr_t)addr & lineMsk) + lineMsk) & ~lineMsk);
    __DSB();
    if(SCB->CCR & SCB_CCR_DC_Msk)
    	SCB_CleanDCache_by_Addr((uint32_t*)start, size);
#endif
}
void __dcache_invalidate_by_addr(void *addr, size_t len){
#if defined (__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    const uint32_t line = 32;
    const uint32_t lineMsk = (line - 1);
    uintptr_t start = (uintptr_t)addr & ~lineMsk;
    size_t  size = ((len + ((uintptr_t)addr & lineMsk) + lineMsk) & ~lineMsk);

    __DSB();
    if(SCB->CCR & SCB_CCR_DC_Msk)
    	SCB_InvalidateDCache_by_Addr((uint32_t*)start, size);
#endif
}
void __dcache_clean_unaligned_lines_by_addr(void *addr, size_t len){
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
    if (end_line != start_line && (((uintptr_t)addr + len) & lineMsk)){
        SCB->DCCMVAC = end_line;
    }

	__DSB();
	__ISB();
#endif
}


// convert transfer level enums to HAL types
static uint32_t __xferLines_to_HALInstrMode(const w25q_xfer_lines_t lines){
	switch(lines){
		case W25Q_XFER_LINES_0: return QSPI_INSTRUCTION_NONE;
		case W25Q_XFER_LINES_1: return QSPI_INSTRUCTION_1_LINE;
		case W25Q_XFER_LINES_2: return QSPI_INSTRUCTION_2_LINES;
		case W25Q_XFER_LINES_4: return QSPI_INSTRUCTION_4_LINES;
	}
	return QSPI_INSTRUCTION_NONE;
}
static uint32_t __xferLines_to_HALAddrMode(const w25q_xfer_lines_t lines){
	switch(lines){
		case W25Q_XFER_LINES_0: return QSPI_ADDRESS_NONE;
		case W25Q_XFER_LINES_1: return QSPI_ADDRESS_1_LINE;
		case W25Q_XFER_LINES_2: return QSPI_ADDRESS_2_LINES;
		case W25Q_XFER_LINES_4: return QSPI_ADDRESS_4_LINES;
	}
	return QSPI_ADDRESS_NONE;
}
static uint32_t __xferLines_to_HALDataMode(const w25q_xfer_lines_t lines){
	switch(lines){
		case W25Q_XFER_LINES_0: return QSPI_DATA_NONE;
		case W25Q_XFER_LINES_1: return QSPI_DATA_1_LINE;
		case W25Q_XFER_LINES_2: return QSPI_DATA_2_LINES;
		case W25Q_XFER_LINES_4: return QSPI_DATA_4_LINES;
	}
	return QSPI_DATA_NONE;
}
static uint32_t __xferLines_to_HALAlterMode(const w25q_xfer_lines_t lines){
	switch(lines){
		case W25Q_XFER_LINES_0: return QSPI_ALTERNATE_BYTES_NONE;
		case W25Q_XFER_LINES_1: return QSPI_ALTERNATE_BYTES_1_LINE;
		case W25Q_XFER_LINES_2: return QSPI_ALTERNATE_BYTES_2_LINES;
		case W25Q_XFER_LINES_4: return QSPI_ALTERNATE_BYTES_4_LINES;
	}
	return QSPI_ALTERNATE_BYTES_NONE;
}

static uint32_t __xferBits_to_HALAddrBits(const w25q_xfer_bits_t bits){
	switch(bits){
		case W25Q_XFER_BITS_8: return QSPI_ADDRESS_8_BITS;
		case W25Q_XFER_BITS_16: return QSPI_ADDRESS_16_BITS;
		case W25Q_XFER_BITS_24: return QSPI_ADDRESS_24_BITS;
		case W25Q_XFER_BITS_32: return QSPI_ADDRESS_32_BITS;
	}
	return QSPI_ADDRESS_8_BITS;
}
static uint32_t __xferBits_to_HALAlterBits(const w25q_xfer_bits_t bits){
	switch(bits){
		case W25Q_XFER_BITS_8: return QSPI_ALTERNATE_BYTES_8_BITS;
		case W25Q_XFER_BITS_16: return QSPI_ALTERNATE_BYTES_16_BITS;
		case W25Q_XFER_BITS_24: return QSPI_ALTERNATE_BYTES_24_BITS;
		case W25Q_XFER_BITS_32: return QSPI_ALTERNATE_BYTES_32_BITS;
	}
	return QSPI_ALTERNATE_BYTES_8_BITS;
}
static QSPI_CommandTypeDef build_qspi_cmd(const w25q_transfer_t *c)
{
	QSPI_CommandTypeDef sCommand = {0};
	sCommand.InstructionMode = __xferLines_to_HALInstrMode(c->instr_lines);
    sCommand.Instruction     = c->instruction;

    sCommand.Address         = c->address;
    sCommand.AddressSize     = __xferBits_to_HALAddrBits(c->addr_bits);
    sCommand.AddressMode     = __xferLines_to_HALAddrMode(c->addr_lines);

    sCommand.AlternateByteMode = __xferLines_to_HALAlterMode(c->alt_lines);
    sCommand.AlternateBytesSize= __xferBits_to_HALAlterBits(c->alt_bits);
	sCommand.AlternateBytes = c->alt_data;

    sCommand.DataMode        = __xferLines_to_HALDataMode(c->data_lines);
    sCommand.NbData          = c->data_len;

    sCommand.DummyCycles     = c->dummy_cycles;

	sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;

	return sCommand;
}





// functions specific for dma use
__IO bool qspi_done = false;
static void __DMA_startTransfer(){
	qspi_done = false;
}

w25q_status_t __wait_dma(void* port_ctx, uint32_t Timeout){
    uint32_t Tickstart = HAL_GetTick();

    while (!qspi_done)
    {
        if ((HAL_GetTick() - Tickstart) > Timeout)
            return W25Q_DMA_TIMEOUT;
        __NOP();
    }
    return W25Q_OK;
}
void HAL_QSPI_RxCpltCallback(QSPI_HandleTypeDef *hqspi){
	 qspi_done = true;
}
void HAL_QSPI_TxCpltCallback(QSPI_HandleTypeDef *hqspi){
	 qspi_done = true;
}





w25q_status_t w25q_port_initialize_hw(void* port_ctx){
	W25Q_ASSERT(port_ctx);
	w25q_stm32_port_ctx_t *ctx = port_ctx;
	if (HAL_QSPI_Init(ctx->hqspi) != HAL_OK)
	    return W25Q_HW_ERROR;
	return W25Q_OK;
}
w25q_status_t w25q_port_deinitialize_hw(void* port_ctx){
	W25Q_ASSERT(port_ctx);
	w25q_stm32_port_ctx_t *ctx = port_ctx;
	if (HAL_QSPI_DeInit(ctx->hqspi) != HAL_OK)
	    return W25Q_HW_ERROR;
	return W25Q_OK;
}
bool w25q_port_is_DMA_enabled(void* port_ctx){
	W25Q_ASSERT(port_ctx);
	w25q_stm32_port_ctx_t *ctx = port_ctx;
	return ctx->hqspi->hdma;
}
bool w25q_port_is_initialized_hw(void* port_ctx){
	W25Q_ASSERT(port_ctx);
	w25q_stm32_port_ctx_t *ctx = port_ctx;


	return (ctx->hqspi->State != HAL_QSPI_STATE_RESET);
}

uint32_t w25q_port_getCLK(void* port_ctx)
{
	W25Q_ASSERT(port_ctx);
    w25q_stm32_port_ctx_t *ctx = port_ctx;

    return HAL_RCC_GetHCLKFreq() / (ctx->hqspi->Init.ClockPrescaler+1);
}

w25q_status_t w25q_port_setCLK(void* port_ctx, uint32_t target_hz)
{
	W25Q_ASSERT(port_ctx);

    w25q_stm32_port_ctx_t *ctx = port_ctx;

    uint32_t prescaler = (HAL_RCC_GetHCLKFreq() + target_hz - 1) / target_hz;
    if (prescaler == 0 || prescaler > 256)
        return W25Q_INVALID_ARG;

    ctx->hqspi->Init.ClockPrescaler = prescaler - 1;

    w25q_status_t st;
    st = w25q_port_deinitialize_hw(port_ctx);
    if(st !=  W25Q_OK) return st;
    st = w25q_port_initialize_hw(port_ctx);
    if(st !=  W25Q_OK) return st;

    return W25Q_OK;
}



w25q_status_t w25q_port_send_command(void* port_ctx, const w25q_transfer_t *c, uint32_t Timeout){
	W25Q_ASSERT(port_ctx);
	W25Q_ASSERT(c);
	w25q_stm32_port_ctx_t *t = port_ctx;

	QSPI_CommandTypeDef sCommand = build_qspi_cmd(c);
	HAL_StatusTypeDef err = HAL_QSPI_Command(t->hqspi, &sCommand, Timeout);
	if(err != HAL_OK) return HAL_ERR_TO_W25QSTATUS(err, SEND_COMMAND);
	return W25Q_OK;
}

w25q_status_t w25q_port_transfer(void* port_ctx, const w25q_transfer_t *c, uint32_t Timeout) {

	W25Q_ASSERT(port_ctx);
    W25Q_ASSERT(c);
    w25q_stm32_port_ctx_t *t = port_ctx;
    HAL_StatusTypeDef err;

    // 1) send instruction+address/alt/dummy
    w25q_status_t status = w25q_port_send_command(port_ctx,c,Timeout);
    if(status != W25Q_OK) return status;

    // 2) decide TX/RX/NONE
    if (c->direction == W25Q_XFER_NONE) return W25Q_OK;


    if (t->hqspi->hdma == NULL || c->prefer_dma == false) {
        if (c->direction == W25Q_XFER_TX) {
        	err = HAL_QSPI_Transmit(t->hqspi, (uint8_t*)c->buf, Timeout);
            if (err != HAL_OK) return HAL_ERR_TO_W25QSTATUS(err, TRANSMIT_DATA);
        } else {
        	err = HAL_QSPI_Receive(t->hqspi, (uint8_t*)c->buf, Timeout);
            if (err != HAL_OK) return HAL_ERR_TO_W25QSTATUS(err, RECEIVE_DATA);
        }
        return W25Q_OK;
    }
    else {
        // DMA path — port does cache maintenance + starts DMA + returns after waiting
        if (c->direction == W25Q_XFER_TX) {
            __dcache_clean_by_addr((void*)c->buf, c->data_len);
            __DMA_startTransfer();
            err = HAL_QSPI_Transmit_DMA(t->hqspi, (uint8_t*)c->buf);
            if (err != HAL_OK) return HAL_ERR_TO_W25QSTATUS(err, DMA_TRANSMIT);
        } else if(c->direction == W25Q_XFER_RX){
        	// quite dangerous if you use close data to this buffer in isr or another task!
            __dcache_clean_unaligned_lines_by_addr((void*)c->buf, c->data_len);
            __DMA_startTransfer();
            err = HAL_QSPI_Receive_DMA(t->hqspi, (uint8_t*)c->buf);
            if (err != HAL_OK) return HAL_ERR_TO_W25QSTATUS(err, DMA_RECEIVE);
        }
        if (__wait_dma(port_ctx, Timeout) != W25Q_OK) return W25Q_DMA_TIMEOUT;
        if(c->direction == W25Q_XFER_RX)
        	__dcache_invalidate_by_addr((void*)c->buf, c->data_len);

        return W25Q_OK;
    }
}

w25q_status_t w25q_port_autoPolling(void* port_ctx, const w25q_transfer_t *c, uint8_t wait_msk, uint8_t wait_val, uint32_t Timeout){

	W25Q_ASSERT(port_ctx);
    W25Q_ASSERT(c);
	w25q_stm32_port_ctx_t *t = port_ctx;

	QSPI_CommandTypeDef sCommand = build_qspi_cmd(c);

    QSPI_AutoPollingTypeDef sConfig;
    memset(&sConfig, 0, sizeof(sConfig));

    sConfig.Mask = wait_msk;
    sConfig.Match = wait_val; // busy should be zeroed
    sConfig.Interval = 0x10;
    sConfig.MatchMode = QSPI_MATCH_MODE_AND;
    sConfig.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
    sConfig.StatusBytesSize = 1;

    HAL_StatusTypeDef err = HAL_QSPI_AutoPolling(t->hqspi, &sCommand, &sConfig, Timeout);
    if(err != HAL_OK) return HAL_ERR_TO_W25QSTATUS(err, POOLING);
    return W25Q_OK;
}


#elif USE_CUSTOM_PORT


__weak w25q_status_t w25q_port_initialize_hw(void* port_ctx){

	 return W25Q_NO_IMPLEMENTATION_ERROR;
}
__weak w25q_status_t w25q_port_deinitialize_hw(void* port_ctx){
	return W25Q_NO_IMPLEMENTATION_ERROR;
}
bool w25q_port_is_DMA_enabled(void* port_ctx){
	return false;
}
bool w25q_port_is_initialized_hw(void* port_ctx){
	return false;
}
__weak uint32_t w25q_port_getCLK(void* port_ctx){
	return 0;
}
__weak w25q_status_t w25q_port_setCLK(void* port_ctx, uint32_t target_hz){
	return W25Q_NO_IMPLEMENTATION_ERROR;
}

__weak w25q_status_t w25q_port_send_command(void* port_ctx, const w25q_transfer_t *c, uint32_t Timeout){
	return W25Q_NO_IMPLEMENTATION_ERROR;
}

__weak w25q_status_t w25q_port_transfer(void* port_ctx, const w25q_transfer_t *c, uint32_t Timeout){
	return W25Q_NO_IMPLEMENTATION_ERROR;
}
__weak w25q_status_t w25q_port_autoPolling(void* port_ctx, const w25q_transfer_t *c, uint8_t wait_msk, uint8_t wait_val, uint32_t Timeout){
	return W25Q_NO_IMPLEMENTATION_ERROR;
}



#endif





