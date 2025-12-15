/*
 * w25q_tests.c
 *
 *  Created on: Oct 4, 2025
 *      Author: denwoken
 */
#include "w25q_tests.h"
#include "stm32f7xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define TEST_DATA_SIZE_32BIT 4096
#define TEST_DATA_SIZE_BYTES (TEST_DATA_SIZE_32BIT*4)
uint32_t InputData[TEST_DATA_SIZE_32BIT];
uint32_t OutputData[TEST_DATA_SIZE_32BIT];
uint32_t DifferData[TEST_DATA_SIZE_32BIT];


extern RNG_HandleTypeDef hrng;

HAL_StatusTypeDef rand_(uint32_t* data){

	return HAL_RNG_GenerateRandomNumber(&hrng, data);
}
uint32_t rand(){
	return HAL_RNG_GetRandomNumber(&hrng);
}

bool W25Q_RunAllTests(w25q_flash_handle_t handle){
	assert_param(handle);

	printf("QSPI runs at clock: %lu.%lu MHz\n", (handle->flash_freq/1000000), (handle->flash_freq%1000000)/1000);

	int errors = 0;
	errors += W25Q_Test_ReadWriteStatusReg(handle);
	errors += W25Q_Test_SectorErase(handle);
	errors += W25Q_Test_PageProgramAndRead(handle);
	errors += W25Q_Test_PageProgramMultiPage(handle);
	errors += W25Q_Test_QuadPageProgram(handle);
	errors += W25Q_Test_FastRead(handle);

	if(errors)
		printf("FAILED: One of Flash tests failed!\n");
	else
		printf("SUCCESS: All Flash tests passed!\n");
	return (bool) errors;
}



bool W25Q_Test_ReadWriteStatusReg(w25q_flash_handle_t handle){
	assert_param(handle);
	printf("=== TEST: ReadWrite Status Registers ===\n");
	w25q_StatusReg1_t sr1 = {0};
	w25q_StatusReg2_t sr2 = {0};
	if (W25Q_ReadStatusReg1(handle, &sr1) != W25Q_OK) {
		printf("ERROR: Failed to read Status Register 1: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}
	printf("SR1 = 0x%02X (BUSY=%d, WEL=%d)\n",
		   *((uint8_t*)&sr1), sr1.BUSY, sr1.WEL);

	if (W25Q_ReadStatusReg2(handle, &sr2) != W25Q_OK) {
		printf("ERROR: Failed to read Status Register 2: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}
	printf("SR2 = 0x%02X (QE=%d)\n", *((uint8_t*)&sr2), sr2.QE);


	sr2.QE = 1;// Меняем QE = 1
	w25q_StatusRegs_t regs = { sr1, sr2 };
	if (W25Q_WriteStatusRegs(handle, &regs) != W25Q_OK) {
		printf("ERROR: Failed to write Status Registers: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}
	if (W25Q_ReadStatusReg2(handle, &sr2) != W25Q_OK) {
		printf("ERROR: Failed to read SR2 after write: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}
	printf("After: SR2=0x%02X (QE=%d)\n", *((uint8_t*)&sr2), sr2.QE);
	if (sr2.QE == 1) {
		printf("SUCCESS: QE bit successfully set!\n\n");
	} else {
		printf("ERROR: QE bit not set correctly!: %s\n\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}
	return false;
}

bool W25Q_Test_SectorErase(w25q_flash_handle_t handle){
	assert_param(handle);
	printf("=== TEST: Sector Erase ===\n");
	uint32_t testAddr = 0; // сотрем первый сектор
	uint8_t buf[4096];

	// Сначала запишем что-то в сектор (например 0xAA)
	memset(buf, 0xAA, sizeof(buf));
	if (W25Q_PageProgram(handle, testAddr, buf, 256) != W25Q_OK) {
		printf("ERROR: Failed to program page before erase: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}

	// Читаем и проверяем, что данные действительно записались (не 0xFF)
	memset(buf, 0, sizeof(buf));
	if (W25Q_ReadData(handle, testAddr, buf, 256) != W25Q_OK) {
		printf("ERROR: Failed to read data before erase: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}
	if (buf[0] == 0xFF) {
		printf("WARNING: Data before erase looks erased already!\n");
	}

	// Теперь стираем сектор
	if (W25Q_SectorErase(handle, testAddr, W25Q_SECTOR_TYPE_4K) != W25Q_OK) {
		printf("ERROR: Failed to erase sector at 0x%06lX: %s\n", (unsigned long)testAddr, W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}

	// Читаем весь сектор и проверяем, что все байты = 0xFF
	memset(buf, 0, sizeof(buf));
	if (W25Q_ReadData(handle, testAddr, buf, sizeof(buf)) != W25Q_OK) {
		printf("ERROR: Failed to read data after erase: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}

	int erasedOk = 1;
	for (int i = 0; i < sizeof(buf); i++) {
		if (buf[i] != 0xFF) {
			printf("ERROR: Sector not erased at offset %d (value=0x%02X)\n", i, buf[i]);
			erasedOk = 0;
			break;
		}
	}

	if (erasedOk) {
		printf("SUCCESS: Sector erased correctly (all 0xFF)\n\n");
	} else return true;

	return false;
}

bool W25Q_Test_PageProgramAndRead(w25q_flash_handle_t handle)
{
	assert_param(handle);
    printf("=== Test: Page Program + Read ===\n");

    memset(InputData, 0xcd, TEST_DATA_SIZE_BYTES);
    memset(OutputData, 0, TEST_DATA_SIZE_BYTES);

    for(int i = 0; i < TEST_DATA_SIZE_32BIT; i++) {
        if(rand_(&InputData[i]) != HAL_OK) {
            printf("ERROR: RNG failed!\n");
            return true;
        }
    }
    printf("Generated %d random words\n", TEST_DATA_SIZE_32BIT);

    if(W25Q_SectorErase(handle, W25Q_PAGENUM_TO_MEMADDR(0), W25Q_SECTOR_TYPE_32K) != W25Q_OK) {
        printf("ERROR: SectorErase failed at addr=0x%06lX: %s\n", (unsigned long)W25Q_PAGENUM_TO_MEMADDR(0), W25Q_STATUS_TO_STR(handle->lastError));
        return true;
    }
    printf("32k SectorErase done\n");


    uint32_t address = W25Q_PAGENUM_TO_MEMADDR(0);
    uint32_t offset = 0;
    int totalPages = TEST_DATA_SIZE_BYTES / W25Q_PAGE_SIZE;

    for(int page = 0; page < totalPages; page++) {
        uint32_t dataSizeInPage = TEST_DATA_SIZE_BYTES - offset;
        if (dataSizeInPage > W25Q_PAGE_SIZE)
            dataSizeInPage = W25Q_PAGE_SIZE;

        if(W25Q_PageProgram(handle, address, (uint8_t*)InputData + offset, dataSizeInPage) != W25Q_OK) {
            printf("ERROR: PageProgram failed at page %d (addr=0x%06lX): %s\n", page, (unsigned long)address, W25Q_STATUS_TO_STR(handle->lastError));
            return true;
        }

//        printf("Page %d programmed, addr=0x%06lX, size=%lu\n",
//               page, (unsigned long)address, (unsigned long)dataSizeInPage);
        offset  += dataSizeInPage;
        address += dataSizeInPage;
    }

    if(W25Q_ReadData(handle, W25Q_PAGENUM_TO_MEMADDR(0), (uint8_t*)OutputData, TEST_DATA_SIZE_BYTES) != W25Q_OK) {
        printf("ERROR: ReadData failed!: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
        return true;
    }
    printf("Read %d bytes from flash\n", TEST_DATA_SIZE_BYTES);

    // 5. Сравнение
    if(memcmp(InputData, OutputData, TEST_DATA_SIZE_BYTES) != 0) {
        printf("ERROR: Data mismatch!\n");
        for(int i = 0; i < 16; i++) {
            printf("[%02d] In=0x%08lX Out=0x%08lX\n", i,
                   (unsigned long)InputData[i], (unsigned long)OutputData[i]);
        }
        return true;
    }

    printf("SUCCESS: Page Program + Read passed!\n\n");
    return false;
}

bool W25Q_Test_PageProgramMultiPage(w25q_flash_handle_t handle){
	assert_param(handle);
	printf("=== Test: Page Program (MultiPage at one time)+ Read ===\n");
	uint32_t address = W25Q_PAGENUM_TO_MEMADDR(0);
	uint32_t offset  = 0;
	uint32_t totalSize = 256*64; // 64 страницы

	if(W25Q_SectorErase(handle, address, W25Q_SECTOR_TYPE_32K) != W25Q_OK) {
	    printf("ERROR: W25Q_SectorErase failed!: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
	    return true;
	}

    for(int i = 0; i < TEST_DATA_SIZE_32BIT; i++) {
        if(rand_(&InputData[i]) != HAL_OK) {
            printf("ERROR: RNG failed!\n");
            return true;
        }
    }

	while(offset < totalSize) {
	    uint32_t writeSize = W25Q_PAGE_SIZE;
	    if(W25Q_PageProgram(handle, address, (uint8_t*)InputData + offset, writeSize) != W25Q_OK) {
	        printf("ERROR: W25Q_PageProgram failed at addr=0x%06lX: %s\n", (unsigned long)address, W25Q_STATUS_TO_STR(handle->lastError));
	        return true;
	    }
	    address += writeSize;
	    offset  += writeSize;
	}

	memset(OutputData, 0, totalSize);
	if(W25Q_ReadData(handle, W25Q_PAGENUM_TO_MEMADDR(0), (uint8_t*)OutputData , totalSize) != W25Q_OK) {
	    printf("ERROR: W25Q_ReadData failed!: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
	    return true;
	}

	if(memcmp((void*)InputData, (void*)OutputData, totalSize) != 0) {
	    printf("ERROR: Data mismatch after read!\n");
	    return true;
	}

	printf("SUCCESS: Multi-page flash test passed!\n\n");
	return false;
}

#define TEST_QUAD_PAGES 16        // количество страниц для теста
bool W25Q_Test_QuadPageProgram(w25q_flash_handle_t handle) {
	assert_param(handle);
    printf("=== TEST: Quad Page Program ===\n");

    // 1. Подготовка тестовых данных
    for(uint32_t i = 0; i < TEST_QUAD_PAGES * W25Q_PAGE_SIZE; i++) {
        InputData[i] = (uint8_t)(i & 0xFF); // просто заполняем шаблоном
    }
    memset(OutputData, 0, sizeof(OutputData));

    // 2. Стираем сектор, где будем писать
    if(W25Q_SectorErase(handle, W25Q_PAGENUM_TO_MEMADDR(0), W25Q_SECTOR_TYPE_4K) != W25Q_OK) {
        printf("ERROR: Sector erase failed!: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
        return true;
    }

    // 3. Записываем постранично через QuadPageProgramm
    uint32_t address = W25Q_PAGENUM_TO_MEMADDR(0);
    for(uint32_t page = 0; page < TEST_QUAD_PAGES; page++) {
        if(W25Q_QuadPageProgram(handle, address, (uint8_t*)InputData + page * W25Q_PAGE_SIZE , W25Q_PAGE_SIZE) != W25Q_OK) {
            printf("ERROR: QuadPageProgram failed at page %lu (addr=0x%06lX): %s\n", page,
            		(unsigned long)address, W25Q_STATUS_TO_STR(handle->lastError));
            return true;
        }
        address += W25Q_PAGE_SIZE;
    }

    // 4. Чтение данных
    if(W25Q_ReadData(handle, W25Q_PAGENUM_TO_MEMADDR(0), (uint8_t*)OutputData, TEST_QUAD_PAGES * W25Q_PAGE_SIZE) != W25Q_OK) {
        printf("ERROR: ReadData failed!: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
        return true;
    }

    // 5. Сравнение
    if(memcmp((void*)InputData, (void*)OutputData, TEST_QUAD_PAGES) != 0) {
        printf("ERROR: Data mismatch!\n");

        for(int i = 0; i < 32; i++) {
            printf("[%06d] In=0x%02lX Out=0x%02lX\n", i, InputData[i], OutputData[i]);
        }
        return true;
    }

    printf("SUCCESS: Quad Page Program test passed!\n\n");
    return false;
}


bool W25Q_Test_FastRead(w25q_flash_handle_t handle) {
	assert_param(handle);
    printf("=== TEST: Fast Read (SIO) ===\n");

    // Подготовка данных
    for(uint32_t i = 0; i < TEST_DATA_SIZE_BYTES; i++) {
        ((uint8_t*)InputData)[i] = (uint8_t)(i & 0xFF);
    }
    memset(OutputData, 0, sizeof(OutputData));

    // Стираем сектор
    if(W25Q_SectorErase(handle, 0, W25Q_SECTOR_TYPE_64K) != W25Q_OK) {
        printf("ERROR: Sector erase failed!: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
        return true;
    }

    // Записываем страницы
    uint32_t addr = 0;
    uint32_t offset = 0;
    while(offset < TEST_DATA_SIZE_BYTES) {
        uint32_t chunk = W25Q_PAGE_SIZE;
        if(W25Q_PageProgram(handle, addr, (uint8_t*)InputData + offset, chunk) != W25Q_OK) {
            printf("ERROR: PageProgram failed at addr=0x%06lX: %s\n", (unsigned long)addr, W25Q_STATUS_TO_STR(handle->lastError));
            return true;
        }
        addr   += chunk;
        offset += chunk;
    }


    if(W25Q_FastRead(handle, 0, (uint8_t*)OutputData, TEST_DATA_SIZE_BYTES, W25Q_FR_MODE_SIO) != W25Q_OK) {
        printf("ERROR: FastRead SIO failed!: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
        return true;
    }
    if(memcmp(InputData, OutputData, TEST_DATA_SIZE_BYTES) != 0) {
        printf("ERROR: Data mismatch in SIO mode!\n");
        return true;
    }
    printf("SUCCESS: Fast Read (SIO) passed!\n\n");



    printf("=== TEST: Fast Read (Dual Output) ===\n");
	memset(OutputData, 0, sizeof(OutputData));
	if(W25Q_FastRead(handle, 0, (uint8_t*)OutputData, TEST_DATA_SIZE_BYTES, W25Q_FR_MODE_DO) != W25Q_OK) {
		printf("ERROR: FastRead DO failed!: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}
	if(memcmp(InputData, OutputData, TEST_DATA_SIZE_BYTES) != 0) {
		printf("ERROR: Data mismatch in DO mode!\n");
		return true;
	}
	printf("SUCCESS: Fast Read (DO) passed!\n\n");



	printf("=== TEST: Fast Read (Quad Output) ===\n");
	memset(OutputData, 0, sizeof(OutputData));
	if(W25Q_FastRead(handle, 0, (uint8_t*)OutputData, TEST_DATA_SIZE_BYTES, W25Q_FR_MODE_QO) != W25Q_OK) {
		printf("ERROR: FastRead QO failed!: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}
	if(memcmp(InputData, OutputData, TEST_DATA_SIZE_BYTES) != 0) {
		printf("ERROR: Data mismatch in QO mode!\n");
		return true;
	}
	printf("SUCCESS: Fast Read (QO) passed!\n\n");



	printf("=== TEST: Fast Read (Dual I/O) ===\n");
	memset(OutputData, 0, sizeof(OutputData));
	if(W25Q_FastRead(handle, 0, (uint8_t*)OutputData, TEST_DATA_SIZE_BYTES, W25Q_FR_MODE_DIO) != W25Q_OK) {
		printf("ERROR: FastRead DIO failed!: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}
	if(memcmp(InputData, OutputData, TEST_DATA_SIZE_BYTES) != 0) {
		printf("ERROR: Data mismatch in DIO mode!\n");
		return true;
	}
	printf("SUCCESS: Fast Read (DIO) passed!\n\n");



	printf("=== TEST: Fast Read (Quad I/O) ===\n");
	memset(OutputData, 0, sizeof(OutputData));
	if(W25Q_FastRead(handle, 0, (uint8_t*)OutputData, TEST_DATA_SIZE_BYTES, W25Q_FR_MODE_QIO) != W25Q_OK) {
		printf("ERROR: FastRead QIO failed!: %s\n", W25Q_STATUS_TO_STR(handle->lastError));
		return true;
	}
	if(memcmp(InputData, OutputData, TEST_DATA_SIZE_BYTES) != 0) {
		printf("ERROR: Data mismatch in QIO mode!\n");
		return true;
	}
	printf("SUCCESS: Fast Read (QIO) passed!\n\n");
	return false;
}






typedef struct {
    const char* name;
    float time_ms;
    float speed_MBps;
    float norm_MBps_per_MHz;
} w25q_benchmark_result_t;

int buffferSize = 4096;
static void run_read_bench(w25q_flash_handle_t handle, w25q_benchmark_result_t* result) {
    const uint32_t size = 1024 * 1024; // 1 MB test
    uint8_t buffer[buffferSize]; // небольшой буфер
    uint32_t address = 0;
    uint32_t start = HAL_GetTick();

    for (uint32_t i = 0; i < size; i += sizeof(buffer)) {
        W25Q_ReadData(handle, address, buffer, sizeof(buffer));
        address += sizeof(buffer);
    }

    uint32_t end = HAL_GetTick();
    result->name = "Read 1MB (SIO)";
    result->time_ms = (float)(end - start);
    result->speed_MBps = (size / (1024.0f*1024.0f)) / (result->time_ms / 1000.0f);
    result->norm_MBps_per_MHz = result->speed_MBps / (handle->flash_freq/1000000.0);
}


static void run_fastread_bench(w25q_flash_handle_t handle, w25q_benchmark_result_t* result, w25q_fr_mode_t mode) {
    const uint32_t size = 1024 * 1024;
    uint8_t buffer[buffferSize];
    uint32_t address = 0;
    uint32_t start = HAL_GetTick();

    for (uint32_t i = 0; i < size; i += sizeof(buffer)) {
        W25Q_FastRead(handle, address, buffer, sizeof(buffer), mode);
        address += sizeof(buffer);
    }

    uint32_t end = HAL_GetTick();
    switch(mode){
    case W25Q_FR_MODE_SIO: result->name = "FastRead 1MB (SIO)"; break;
    case W25Q_FR_MODE_DO: result->name = "FastRead 1MB (DO)"; break;
    case W25Q_FR_MODE_QO: result->name = "FastRead 1MB (QO)"; break;
    case W25Q_FR_MODE_DIO: result->name = "FastRead 1MB (DIO)"; break;
    case W25Q_FR_MODE_QIO: result->name = "FastRead 1MB (QIO)"; break;
    default: result->name = "FastRead 1MB (QIO)"; break;
    }

    result->time_ms = (float)(end - start);
    result->speed_MBps = (size / (1024.0*1024.0)) / (result->time_ms / 1000.0);
    result->norm_MBps_per_MHz = result->speed_MBps / (handle->flash_freq/1000000.0);
}


static void run_write_bench(w25q_flash_handle_t handle, w25q_benchmark_result_t* result) {
    const uint32_t size = 1024 * 1024;
    uint8_t buffer[buffferSize];
    memset(buffer, 0xAA, sizeof(buffer));
    uint32_t address = 0;

    uint32_t start = HAL_GetTick();
    for (uint32_t i = 0; i < size; i += sizeof(buffer)) {
        W25Q_PageProgram(handle, address, buffer, sizeof(buffer));
        address += sizeof(buffer);
    }
    uint32_t end = HAL_GetTick();

    result->name = "PageProgram 1MB (SIO)";
    result->time_ms = (float)(end - start);
    result->speed_MBps = (size / (1024.0f*1024.0f)) / (result->time_ms / 1000.0f);
    result->norm_MBps_per_MHz = result->speed_MBps / (handle->flash_freq/1000000.0);
}

static void run_quadwrite_bench(w25q_flash_handle_t handle, w25q_benchmark_result_t* result) {
    const uint32_t size = 1024 * 1024;
    uint8_t buffer[buffferSize];
    memset(buffer, 0x55, sizeof(buffer));
    uint32_t address = 0;

    uint32_t start = HAL_GetTick();
    for (uint32_t i = 0; i < size; i += sizeof(buffer)) {
        W25Q_QuadPageProgram(handle, address, buffer, sizeof(buffer));
        address += sizeof(buffer);
    }
    uint32_t end = HAL_GetTick();

    result->name = "QPageProgram 1MB (QO)";
    result->time_ms = (float)(end - start);
    result->speed_MBps = (size / (1024.0f*1024.0f)) / (result->time_ms / 1000.0f);
    result->norm_MBps_per_MHz = result->speed_MBps / (handle->flash_freq/1000000.0);
}

static void run_erase_bench(w25q_flash_handle_t handle, w25q_benchmark_result_t* result, w25q_sector_t type, const char* name) {
    uint32_t address = 0;
    uint32_t start = HAL_GetTick();
    W25Q_SectorErase(handle, address, type);
    uint32_t end = HAL_GetTick();

    result->name = name;
    result->time_ms = (float)(end - start);

    uint32_t size_bytes = 0;
    switch (type) {
    case W25Q_SECTOR_TYPE_4K: size_bytes = 4 * 1024; break;
    case W25Q_SECTOR_TYPE_32K: size_bytes = 32 * 1024; break;
    case W25Q_SECTOR_TYPE_64K: size_bytes = 64 * 1024; break;
    case W25Q_SECTOR_TYPE_ALLCHIP: size_bytes = (1 << handle->info.Capacity); break; // Capacity = log2(size_bytes)
    default: size_bytes = 0; break;
    }

    if (size_bytes > 0) {
        result->speed_MBps = (size_bytes / (1024.0f * 1024.0f)) / (result->time_ms / 1000.0f);
        result->norm_MBps_per_MHz = result->speed_MBps / (handle->flash_freq / 1000000.0f);
    } else {
        result->speed_MBps = 0;
        result->norm_MBps_per_MHz = 0;
    }
}


static void run_random_read_latency_bench(w25q_flash_handle_t handle, const uint32_t read_size, w25q_benchmark_result_t* result, const char* name) {
    const uint32_t total_ops = 100;       // количество случайных выборок
    uint8_t buffer[read_size];

    uint32_t flash_size = (1U << handle->info.Capacity);
    uint32_t address_mask = flash_size - read_size;

    uint32_t start = HAL_GetTick();

    for (uint32_t i = 0; i < total_ops; i++) {
        uint32_t addr = rand() & address_mask;
        W25Q_FastRead(handle, addr, buffer, read_size, W25Q_FR_MODE_BEST_AVAILABLE);
    }

    uint32_t end = HAL_GetTick();

    result->name = name;
    result->time_ms = (float)(end - start);

    // Среднее время одного чтения в микросекундах
    float avg_us = (result->time_ms * 1000.0f) / total_ops;

    // Скорость "условная", просто чтобы не оставлять 0
    float total_bytes = (float)(total_ops * read_size);
    result->speed_MBps = (total_bytes / (1024.0f * 1024.0f)) / (result->time_ms / 1000.0f);
    result->norm_MBps_per_MHz = result->speed_MBps / (handle->flash_freq / 1000000.0f);

    printf("Random read avg latency = %.2f us per %luB read\n", avg_us, (unsigned long)read_size);
}

static void run_random_write_latency_bench(w25q_flash_handle_t handle, const uint32_t write_size, w25q_benchmark_result_t* result, const char* name) {
    const uint32_t total_ops = 100;       // количество случайных выборок
    uint8_t buffer[write_size];

    uint32_t flash_size = (1U << handle->info.Capacity);
    uint32_t address_mask = flash_size - write_size;

    uint32_t start = HAL_GetTick();

    for (uint32_t i = 0; i < total_ops; i++) {
        uint32_t addr = rand() & address_mask;
        W25Q_QuadPageProgram(handle, addr, buffer, write_size);
    }

    uint32_t end = HAL_GetTick();


    result->name = name;
    result->time_ms = (float)(end - start);

    // Среднее время одного чтения в микросекундах
    float avg_us = (result->time_ms * 1000.0f) / total_ops;

    // Скорость "условная", просто чтобы не оставлять 0
    float total_bytes = (float)(total_ops * write_size);
    result->speed_MBps = (total_bytes / (1024.0f * 1024.0f)) / (result->time_ms / 1000.0f);
    result->norm_MBps_per_MHz = result->speed_MBps / (handle->flash_freq / 1000000.0f);

    printf("Random write avg latency = %.2f us per %luB write\n", avg_us, (unsigned long)write_size);
}


void W25Q_RunAllBenchMark(w25q_flash_handle_t handle) {
	assert_param(handle);
	printf("Bench runs at sclk speed = %f\n", handle->flash_freq/1000000.0);
	const uint8_t resSize = 20;
	uint8_t resultInc = 0;
    w25q_benchmark_result_t results[resSize];


    printf("=== W25Q Benchmarks ===  SCLK = %2.2f MHz; buffferSize = %d B; DMA = %s\n",
    		handle->flash_freq/1000000.0, buffferSize, (handle->hqspi->hdma)?"true":"false");

    // === WRITE ===
	run_write_bench(handle, &results[resultInc++]);
	run_quadwrite_bench(handle, &results[resultInc++]);

	// === READ ===
	run_read_bench(handle, &results[resultInc++]);
	run_fastread_bench(handle, &results[resultInc++], W25Q_FR_MODE_DO);
	run_fastread_bench(handle, &results[resultInc++], W25Q_FR_MODE_QO);
	run_fastread_bench(handle, &results[resultInc++], W25Q_FR_MODE_DIO);
	run_fastread_bench(handle, &results[resultInc++], W25Q_FR_MODE_QIO);

	// === ERASE ===
	run_erase_bench(handle, &results[resultInc++], W25Q_SECTOR_TYPE_4K, "Erase 4KB sector");
	run_erase_bench(handle, &results[resultInc++], W25Q_SECTOR_TYPE_32K, "Erase 32KB block");
	run_erase_bench(handle, &results[resultInc++], W25Q_SECTOR_TYPE_64K, "Erase 64KB block");
	run_erase_bench(handle, &results[resultInc++], W25Q_SECTOR_TYPE_ALLCHIP, "Erase full chip");

    run_random_read_latency_bench(handle, W25Q_PAGE_SIZE,  &results[resultInc++], "Random Read Latency (QIO) 256B");
    run_random_read_latency_bench(handle, 4096,  &results[resultInc++], "Random Read Latency (QIO) 4KB");
    run_random_write_latency_bench(handle, W25Q_PAGE_SIZE,  &results[resultInc++], "Random Write Latency (QO) 256B");
    run_random_write_latency_bench(handle, 4096,  &results[resultInc++], "Random Write Latency (QO) 4KB");


    for (int i = 0; i < resultInc; i++) {
        printf("%-29s \tTime = %.1f ms   \tSpeed = %.3f MB/s\tNorm = %.6f MB/s/MHz\n",
            results[i].name,
            results[i].time_ms,
            results[i].speed_MBps,
            results[i].norm_MBps_per_MHz);
    }
    printf("\n");


}





