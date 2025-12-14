/*
 * w25q_port.c
 *
 *  Created on: Dec 13, 2025
 *      Author: denwoken
 */


#include "w25q_port.h"
#include "w25q_transfer_level.h"



void w25q_port_dcache_clean(void *addr, size_t len){
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
void w25q_port_dcache_invalidate(void *addr, size_t len){
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
void w25q_port_dcache_clean_unaligned(void *addr, size_t len){
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





static uint32_t __xferLines_to_HALInstrMode(const w25q_xfer_lines_t lines){
	switch(lines){
		case W25Q_XFER_LINES_0: return QSPI_INSTRUCTION_NONE;
		case W25Q_XFER_LINES_1: return QSPI_INSTRUCTION_1_LINE;
		case W25Q_XFER_LINES_2: return QSPI_INSTRUCTION_2_LINES;
		case W25Q_XFER_LINES_4: return QSPI_INSTRUCTION_4_LINES;
	}
}
static uint32_t __xferLines_to_HALAddrMode(const w25q_xfer_lines_t lines){
	switch(lines){
		case W25Q_XFER_LINES_0: return QSPI_ADDRESS_NONE;
		case W25Q_XFER_LINES_1: return QSPI_ADDRESS_1_LINE;
		case W25Q_XFER_LINES_2: return QSPI_ADDRESS_2_LINES;
		case W25Q_XFER_LINES_4: return QSPI_ADDRESS_4_LINES;
	}
}
static uint32_t __xferLines_to_HALDataMode(const w25q_xfer_lines_t lines){
	switch(lines){
		case W25Q_XFER_LINES_0: return QSPI_DATA_NONE;
		case W25Q_XFER_LINES_1: return QSPI_DATA_1_LINE;
		case W25Q_XFER_LINES_2: return QSPI_DATA_2_LINES;
		case W25Q_XFER_LINES_4: return QSPI_DATA_4_LINES;
	}
}
static uint32_t __xferAddrBits_to_HALAddrBits(const w25q_xfer_addr_bits_t addr_bits){
	switch(addr_bits){
		case W25Q_XFER_ADDR_8: return QSPI_ADDRESS_8_BITS;
		case W25Q_XFER_ADDR_16: return QSPI_ADDRESS_16_BITS;
		case W25Q_XFER_ADDR_24: return QSPI_ADDRESS_24_BITS;
		case W25Q_XFER_ADDR_32: return QSPI_ADDRESS_32_BITS;
	}
}
static QSPI_CommandTypeDef build_qspi_cmd(const w25q_transfer_t *c)
{
	QSPI_CommandTypeDef sCommand = {0};
	sCommand.InstructionMode = __xferLines_to_HALInstrMode(c->instr_lines);
    sCommand.Instruction     = c->instruction;

    sCommand.Address         = c->address;
    sCommand.AddressSize     = __xferAddrBits_to_HALAddrBits(c->addr_bits);
    sCommand.AddressMode     = __xferLines_to_HALAddrMode(c->addr_lines);

    sCommand.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;

    sCommand.DataMode        = __xferLines_to_HALDataMode(c->data_lines);
    sCommand.NbData          = c->data_len;

    sCommand.DummyCycles     = c->dummy_cycles;

	sCommand.DdrMode             = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;

	return sCommand;
}





w25q_status_t w25q_port_send_command(void *port_ctx, const w25q_transfer_t *c, uint32_t Timeout){
	assert_param(port_ctx);
	assert_param(c);
	w25q_stm32_port_ctx_t *t = port_ctx;

	QSPI_CommandTypeDef sCommand = build_qspi_cmd(c);
	HAL_StatusTypeDef err = HAL_QSPI_Command(t->hqspi, &sCommand, Timeout);
	if(err != HAL_OK) return HAL_ERR_TO_W25QSTATUS(err, SEND_COMMAND);
	return W25Q_OK;
}


/*
w25q_status_t w25q_port_transmit(void *port_ctx,  const w25q_transfer_t *c, uint32_t Timeout){

	assert_param(port_ctx);
	assert_param(c);
	assert_param(c->buf);
	w25q_stm32_port_ctx_t *t = port_ctx;

	w25q_status_t status = w25q_port_send_command(port_ctx, c, Timeout);
	if(status!=W25Q_OK) return status;


	HAL_StatusTypeDef err = HAL_QSPI_Transmit(t->hqspi, (uint8_t*)c->buf, Timeout);
	switch(err)	{
		case HAL_OK: 		return W25Q_OK;
		case HAL_TIMEOUT: 	return W25Q_TRANSMIT_DATA_TIMEOUT;
		case HAL_ERROR: 	return W25Q_TRANSMIT_DATA_ERROR;
		case HAL_BUSY: 		return W25Q_TRANSMIT_DATA_ERROR;
	}
	return W25Q_ERROR;

};
w25q_status_t w25q_port_receive(void *port_ctx, const w25q_transfer_t *c, uint32_t Timeout){

	assert_param(port_ctx);
	assert_param(c);
	assert_param(c->buf);
	w25q_stm32_port_ctx_t *t = port_ctx;

	w25q_status_t status = w25q_port_send_command(port_ctx, c, Timeout);
	if(status!=W25Q_OK) return status;


	HAL_StatusTypeDef err = HAL_QSPI_Receive(t->hqspi, (uint8_t*)c->buf, Timeout);
	switch(err)	{
		case HAL_OK: 		return W25Q_OK;
		case HAL_TIMEOUT: 	return W25Q_RECEIVE_DATA_TIMEOUT;
		case HAL_ERROR: 	return W25Q_RECEIVE_DATA_ERROR;
		case HAL_BUSY: 		return W25Q_RECEIVE_DATA_ERROR;
	}
	return W25Q_ERROR;
}

*/


w25q_status_t w25q_port_transfer(void *port_ctx, const w25q_transfer_t *c, uint32_t Timeout) {
    assert_param(port_ctx);
    assert_param(c);
    w25q_stm32_port_ctx_t *t = port_ctx;
    HAL_StatusTypeDef err;

    // 1) send instruction+address/alt/dummy
    w25q_status_t status = w25q_port_send_command(port_ctx,c,Timeout);
    if(status != W25Q_OK) return W25Q_OK;

    // 2) decide TX/RX/NONE and Polling path
    if (c->direction == W25Q_XFER_NONE) return W25Q_OK;


    if (t->hqspi->hdma == NULL ) {//|| !should_use_dma(t, c)
        if (c->direction == W25Q_XFER_TX) {
        	err = HAL_QSPI_Transmit(t->hqspi, (uint8_t*)c->buf, Timeout);
            if (err != HAL_OK) return HAL_ERR_TO_W25QSTATUS(err, TRANSMIT_DATA);
        } else {
        	err = HAL_QSPI_Receive(t->hqspi, (uint8_t*)c->buf, Timeout);
            if (err != HAL_OK) return HAL_ERR_TO_W25QSTATUS(err, RECEIVE_DATA);
        }
        return W25Q_OK;
    }
//    else {
//        // DMA path — port does cache maintenance + starts DMA + returns after waiting
//        if (c->direction == W25Q_XFER_TX) {
//            dcache_clean_by_addr((void*)c->buf, c->data_len);
//            W25Q_DMA_startTransfer(); // твоя функция или внутр.флаг
//            if (HAL_QSPI_Transmit_DMA(t->hqspi, (uint8_t*)c->buf) != HAL_OK) return W25Q_TRANSMIT_DATA_ERROR;
//        } else {
//            dcache_clean_unaligned_lines_by_addr((void*)c->buf, c->data_len);
//            W25Q_DMA_startTransfer();
//            if (HAL_QSPI_Receive_DMA(t->hqspi, (uint8_t*)c->buf) != HAL_OK) return W25Q_RECEIVE_DATA_ERROR;
//        }
//        if (w25q_port_wait_dma(port_ctx, Timeout) != W25Q_OK) return W25Q_DMA_TIMEOUT;
//        if (c->direction == W25Q_XFER_RX) dcache_invalidate_by_addr((void*)c->buf, c->data_len);
//        return W25Q_OK;
//    }
}





w25q_status_t w25q_port_autoPooling(void *port_ctx, const w25q_transfer_t *c, uint8_t wait_msk, uint8_t wait_val, uint32_t Timeout){
    assert_param(port_ctx);
    assert_param(c);
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








