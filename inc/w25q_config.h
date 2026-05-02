/*
 * w25q_config.h
 *
 *  Created on: Dec 13, 2025
 *      Author: denwoken
 */
#pragma once



#define DMA_ENABLE 1
#define DMA_THRESHOLD 32

#define W25Q_SAFE_INIT_CLK_HZ   (1000000UL)

#define USE_STM32_PORT_F7 1
#define USE_CUSTOM_PORT 0

/*
 0 - no assert
 1 - assert_param stm32 asssert with function assert_failed
 2 - custom imlementation define it yourserlf
#define W25Q_ASSERT(expr) ((void)0U)
 * */
#define ASSERT_IMPLEMENTATION 1








