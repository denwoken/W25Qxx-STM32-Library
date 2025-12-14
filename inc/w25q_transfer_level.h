#pragma once



typedef enum {
	W25Q_XFER_LINES_NONE = 0,
	W25Q_XFER_LINES_0 = W25Q_XFER_LINES_NONE,
    W25Q_XFER_LINES_1 = 1,
    W25Q_XFER_LINES_2 = 2,
    W25Q_XFER_LINES_4 = 4,
} w25q_xfer_lines_t;

typedef enum {
	W25Q_XFER_BITS_8   = 8,
	W25Q_XFER_BITS_16  = 16,
    W25Q_XFER_BITS_24  = 24,
	W25Q_XFER_BITS_32  = 32,
} w25q_xfer_bits_t;



typedef enum {
    W25Q_XFER_NONE = 0,
    W25Q_XFER_TX,
    W25Q_XFER_RX,
} w25q_xfer_dir_t;






typedef struct {
    uint8_t instruction;
    w25q_xfer_lines_t instr_lines;

    uint32_t address;
    w25q_xfer_bits_t addr_bits;
    w25q_xfer_lines_t addr_lines;

    uint32_t alt_data;
    w25q_xfer_bits_t  alt_bits;
    w25q_xfer_lines_t alt_lines;

    uint8_t dummy_cycles;

    w25q_xfer_lines_t data_lines;
    w25q_xfer_dir_t   direction;
    void *buf;
    uint32_t data_len;

} w25q_transfer_t;


















