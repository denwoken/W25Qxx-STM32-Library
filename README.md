# W25Qxx-STM32-Library

STM32 driver for Winbond W25Q serial NOR flash focused on `STM32F7 + HAL QSPI`.

The library exposes a compact high-level API for initialization, JEDEC/UUID readout, status register handling, page program, erase, and fast-read modes. The included STM32F7 port also supports DMA transfers and Cortex-M7 D-Cache maintenance for higher throughput.

## Features

- JEDEC ID and 64-bit unique ID readout
- Status register read/write helpers
- Standard read and page program operations
- Quad page program support
- Fast read modes: `SIO`, `DO`, `QO`, `DIO`, `QIO`
- Sector erase: `4 KB`, `32 KB`, `64 KB`, and chip erase
- DMA-aware transfer path with **cache maintenance** for STM32F7
- Test and benchmark helpers in [`tests/`](tests)

## Current Scope

- Included hardware port: `STM32F7 HAL QSPI`
- Custom ports are possible through the **weak** `w25q_port_*()` hooks
- The driver uses 24-bit addressing in the current implementation
- There is no ready-made CubeMX example project in this repository yet

## Repository Layout

- [`inc/`](inc) public headers and configuration
- [`src/`](src) core driver and STM32F7 port layer
- [`tests/`](tests) functional tests and benchmark helpers
- [`doc/w25q64.pdf`](doc/w25q64.pdf) Winbond reference datasheet

## Configuration

Main build-time options live in [`inc/w25q_config.h`](inc/w25q_config.h):

- `DMA_ENABLE`: enables DMA path when the QSPI handle has a DMA channel
- `DMA_THRESHOLD`: minimum transfer size for DMA
- `W25Q_SAFE_INIT_CLK_HZ`: temporary low clock used during `W25Q_init()`
- `USE_STM32_PORT_F7`: enables the bundled STM32F7 HAL QSPI port
- `USE_CUSTOM_PORT`: enables user-defined port functions
- `ASSERT_IMPLEMENTATION`: selects how `W25Q_ASSERT()` behaves

## Quick Start

1. Add `inc/` to include paths and compile `src/w25q.c` plus `src/w25q_port.c`.
2. Initialize `QSPI_HandleTypeDef` in CubeMX/HAL as usual.
3. If you want DMA acceleration, link a DMA handle to `hqspi`.
4. Create a port context and a flash handle.
5. Call `W25Q_init()` once before any flash access.

Minimal initialization example:

```c
#include "w25q.h"
#include "w25q_port.h"

extern QSPI_HandleTypeDef hqspi;

static w25q_stm32_port_ctx_t flash_port = {
    .hqspi = &hqspi,
};

static w25q_flash_t flash = {
    .port_ctx = &flash_port,
};

void ExtFlash_Init(void)
{
    if (W25Q_init(&flash) != W25Q_OK) {
        Error_Handler();
    }
}
```

After initialization, the populated `flash.info` structure contains:

- `Manufacturer`
- `MemoryType`
- `Capacity`
- `CapacityBytes`
- `uuid[8]`

## Basic Usage

Read identification:

```c
uint8_t jedec_id[3];
uint8_t uuid[8];

W25Q_ReadID(&flash, jedec_id);
W25Q_ReadUUID(&flash, uuid);
```

Erase, program, and read one page:

```c
uint8_t tx[W25Q_PAGE_SIZE];
uint8_t rx[W25Q_PAGE_SIZE];

memset(tx, 0x5A, sizeof(tx));

W25Q_SectorErase(&flash, 0x000000, W25Q_SECTOR_TYPE_4K);
W25Q_PageProgram(&flash, 0x000000, tx, sizeof(tx));
W25Q_ReadData(&flash, 0x000000, rx, sizeof(rx));
```

Fast read using the best available mode:

```c
uint8_t buf[4096];

W25Q_FastRead(&flash, 0x000000, buf, sizeof(buf), W25Q_FR_MODE_BEST_AVAILABLE);
```

## API Overview

| Function | Purpose |
| --- | --- |
| `W25Q_init()` | Initializes the port, temporarily lowers flash clock, reads device info |
| `W25Q_ReadInfo()` | Reads JEDEC fields and unique ID into `w25q_info_t` |
| `W25Q_ReadData()` | Standard 1-line read |
| `W25Q_FastRead()` | Fast read in `SIO/DO/QO/DIO/QIO` mode |
| `W25Q_PageProgram()` | Standard page program |
| `W25Q_QuadPageProgram()` | Quad data-line page program |
| `W25Q_SectorErase()` | Erases `4K`, `32K`, `64K`, or full chip |
| `W25Q_ReadStatusReg1/2()` | Reads flash status registers |
| `W25Q_WriteStatusRegs()` | Writes status register pair |
| `W25Q_getCLK()` / `W25Q_setCLK()` | Reads or updates QSPI serial clock |

## Tests And Benchmarks

Optional helper entry points are declared in [`tests/w25q_tests.h`](tests/w25q_tests.h):

- `W25Q_RunAllTests()`
- `W25Q_RunAllBenchMark()`

The test code covers:

- status register access
- erase verification
- single-page and multi-page program/read flows
- quad page program
- fast-read modes

## Benchmark Reference

The following numbers were taken from the benchmark log provided for `NUCLEO-144 STM32F767` with:

- QSPI serial clock: `72 MHz`
- transfer buffer: `4096 B`
- four runtime combinations: `DMA on/off` and `I-Cache + D-Cache on/off`

Summary of the most relevant results:

| Runtime setup | FastRead QIO 1 MB | Quad Page Program 1 MB | Random read 256 B | Random write 4 KB | Full chip erase |
| --- | ---: | ---: | ---: | ---: | ---: |
| No cache, DMA off | `1.067 MB/s` | `1.046 MB/s` | `210 us` | `3380 us` | `4583 ms` |
| No cache, DMA on | `20.408 MB/s` | `4.587 MB/s` | `40 us` | `500 us` | `4594 ms` |
| I-Cache + D-Cache, DMA off | `2.732 MB/s` | `2.033 MB/s` | `90 us` | `1570 us` | `4588 ms` |
| I-Cache + D-Cache, DMA on | `26.316 MB/s` | `4.902 MB/s` | `20 us` | `440 us` | `4589 ms` |

Takeaways:

- DMA is the biggest performance lever for reads.
- Enabling cache improves both polling and DMA paths on STM32F7.
- Erase timings are dominated by the flash device itself, so DMA/cache do not change them much.
- The best measured sequential read in this setup was `26.316 MB/s` with `QIO + DMA + cache`.

## Notes And Limitations

- The bundled port is currently written for `STM32F7 HAL QSPI`, not generic STM32 SPI.
- `W25Q_PageProgram()` and `W25Q_QuadPageProgram()` should be treated as page-oriented operations. Application code should split writes on `W25Q_PAGE_SIZE` boundaries.
- Quad operations automatically ensure the `QE` bit is set before use.
- If you enable `USE_CUSTOM_PORT`, you must implement the `w25q_port_*()` functions yourself.
- The current codebase uses 24-bit addressing; if you target larger parts or 4-byte address mode, review the address limits first.

