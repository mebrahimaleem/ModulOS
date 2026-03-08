/* fis.h - SATA Frame Information Structure */
/* Copyright (C) 2026  Ebrahim Aleem
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>
*/

#ifndef DRIVERS_SATA_FIS_H
#define DRIVERS_SATA_FIS_H

#include <stdint.h>

#define SATA_FIS_TYPE_H2D				0x27
#define SATA_FIS_TYPE_D2H				0x34
#define SATA_FIS_TYPE_SMA_SETUP	0x41
#define SATA_FIS_TYPE_PIO_SETUP	0x5F
#define SATA_FIS_TYPE_SET_DEV		0xA1

#define SATA_FIS_H2D_C					0x80

#define SATA_FIS_CMD_DMA_READ_EXT			0x25
#define SATA_FIS_CMD_DMA_WRITE_EXT		0x35
#define SATA_FIS_CMD_FLUSH_CACHE_EXT	0xEA
#define SATA_FIS_CMD_IDENT						0xEC

#define SATA_FIS_CMD_W					0x40

struct sata_fis_type_h2d_t {
	uint8_t fis_type;
	uint8_t flag;
	uint8_t cmd;
	uint8_t feat_lo;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t dev;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t feat_hi;

	uint8_t count_lo;
	uint8_t count_hi;
	uint8_t icc;
	uint8_t cntr;

	uint32_t resv0;
};

struct sata_fis_type_d2h_t {
	uint8_t fis_type;
	uint8_t flag;
	uint8_t sts;
	uint8_t err;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t dev;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t resv0;

	uint8_t count_lo;
	uint8_t count_hi;
	uint16_t resv1;

	uint32_t resv2;
} __attribute__((packed));

struct sata_fis_dma_setup_t {
	uint8_t fis_type;
	uint8_t flag;
	uint16_t resv0;

	uint64_t dma_buf_id;

	uint32_t resv1;

	uint32_t dma_buf_off;

	uint32_t trans_count;

	uint32_t resv3;
} __attribute__((packed));

struct sata_fis_pio_setupt_t {
	uint8_t fis_type;
	uint8_t flag;
	uint8_t sts;
	uint8_t err;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t dev;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t resv0;

	uint8_t count_lo;
	uint8_t count_hi;
	uint8_t resv1;
	uint8_t e_sts;

	uint16_t trans_count;
	uint16_t resv2;
} __attribute__((packed));

#endif /* DRIVERS_SATA_FIS_H */
