#pragma once



typedef enum {
	W25Q_XFER_LINES_NONE = 0,
	W25Q_XFER_LINES_0 = W25Q_XFER_LINES_NONE,
    W25Q_XFER_LINES_1 = 1,
    W25Q_XFER_LINES_2 = 2,
    W25Q_XFER_LINES_4 = 4,
} w25q_xfer_lines_t;

typedef enum {
	W25Q_XFER_ADDR_8   = 8,
	W25Q_XFER_ADDR_16   = 16,
    W25Q_XFER_ADDR_24   = 24,
    W25Q_XFER_ADDR_32   = 32,
} w25q_xfer_addr_bits_t;

typedef enum {
    W25Q_XFER_NONE = 0,
    W25Q_XFER_TX,
    W25Q_XFER_RX,
//    W25Q_XFER_TX_RX,
} w25q_xfer_dir_t;






typedef struct {
    uint8_t instruction;
    w25q_xfer_lines_t instr_lines;

    uint32_t address;
    w25q_xfer_addr_bits_t addr_bits;
    w25q_xfer_lines_t addr_lines;

//    uint32_t alt_bytes;
//    uint8_t  alt_bytes_len;  // 0..4
//    w25q_xfer_lines_t alt_lines;

    uint8_t dummy_cycles;

    w25q_xfer_lines_t data_lines;
    w25q_xfer_dir_t   direction;

    void *buf;
//    const void *tx_buf;
    uint32_t data_len;

//    uint8_t sioo;                         // send instruction only once
} w25q_transfer_t;


















