/*
 * w25q_tests.h
 *
 *  Created on: Oct 4, 2025
 *      Author: denwoken
 */

#pragma once
#include <stdbool.h>
#include "w25q.h"

bool W25Q_RunAllTests(w25q_flash_handle_t handle);

bool W25Q_Test_ReadWriteStatusReg(w25q_flash_handle_t handle);
bool W25Q_Test_SectorErase(w25q_flash_handle_t handle);
bool W25Q_Test_PageProgramAndRead(w25q_flash_handle_t handle);
bool W25Q_Test_PageProgramMultiPage(w25q_flash_handle_t handle);
bool W25Q_Test_QuadPageProgram(w25q_flash_handle_t handle);
bool W25Q_Test_FastRead(w25q_flash_handle_t handle);





void W25Q_RunAllBenchMark(w25q_flash_handle_t handle);

