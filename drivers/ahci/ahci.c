/* ahci.c - Advanced Host Controller Interface driver */
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

#include <stdint.h>

#include <ahci/ahci.h>

#include <sata/fis.h>

#include <pcie/pcie.h>

#include <disk/disk.h>

#include <kernel/core/logging.h>
#include <kernel/core/scheduler.h>
#include <kernel/core/process.h>
#include <kernel/core/alloc.h>
#include <kernel/core/mm.h>
#include <kernel/core/paging.h>
#include <kernel/core/panic.h>
#include <kernel/core/time.h>
#include <kernel/core/lock.h>

#include <kernel/lib/kmemset.h>
#include <kernel/lib/kmemcpy.h>

#define CAP_OFF						0x00
#define GHC_OFF						0x04
#define PI_OFF						0x0C
#define PXCLB_OFF(port)		(0x100 + (0x80 * port) + 0x00)
#define PXCLBU_OFF(port)	(0x100 + (0x80 * port) + 0x04)
#define PXFB_OFF(port)		(0x100 + (0x80 * port) + 0x08)
#define PXFBU_OFF(port)		(0x100 + (0x80 * port) + 0x0C)
#define PXIS_OFF(port)		(0x100 + (0x80 * port) + 0x10)
#define PXCMD_OFF(port)		(0x100 + (0x80 * port) + 0x18)
#define PXTFD_OFF(port)		(0x100 + (0x80 * port) + 0x20)
#define PXSSTS_OFF(port)	(0x100 + (0x80 * port) + 0x28)
#define PXSCTL_OFF(port)	(0x100 + (0x80 * port) + 0x2C)
#define PXSERR_OFF(port)	(0x100 + (0x80 * port) + 0x30)
#define PXCI_OFF(port)		(0x100 + (0x80 * port) + 0x38)

#define CAP_NCS_MASK	0x1F00u
#define CAP_NCS_SHFT	8
#define CAP_S64A			0x80000000u

#define GHC_AE	0x80000000u
#define GHC_IE	0x1u

#define IS_TFES	0x40000000u

#define CMD_ST	0x0001u
#define CMD_FRE	0x0010u
#define CMD_FR	0x4000u
#define CMD_CR	0x8000u

#define TFD_STS_CON_MASK	0x88

#define SERR_CLEAR	0xFFF0F03

#define SCTL_DET_MSK	0x000Fu
#define SCTL_DET_RST	0x0001u

#define SSTS_DET_MASK	0x000Fu
#define SSTS_DET_EST	0x0003u

#define CL_SIZE		1024
#define FIS_SIZE	256

#define SLOT_NO_SLOT	0xFF

#define PRDT_DBC_I	0x80000000u

#define SECTOR_SIZE	512

struct ahci_recv_fis_t {
	struct sata_fis_dma_setup_t dsfis;
	uint32_t resv0;
	struct sata_fis_pio_setupt_t psfis;
	uint32_t resv1[3];
	struct sata_fis_type_d2h_t rfis;
	uint32_t resv2;
	uint64_t sdbfis;
	uint8_t ufis[64];
	uint8_t resv3[96];
} __attribute__((packed));

struct ahci_command_header_t {
	uint16_t flg;
	uint16_t prdtl;
	uint32_t prdbc;
	uint32_t ctba0;
	uint32_t ctba_u0;
	uint32_t resv0;
	uint32_t resv1;
	uint32_t resv2;
	uint32_t resv3;
} __attribute__((packed));

struct ahci_prdt_t {
	uint32_t dba;
	uint32_t dbau;
	uint32_t resv;
	uint32_t dbc_i;
} __attribute__((packed));

struct ahci_command_table_t {
	union {
		uint8_t _64[64];
		struct sata_fis_type_h2d_t h2d;
	} cfis;
	uint8_t acmd[16];
	uint8_t resv[48];
	struct ahci_prdt_t prdt[];
} __attribute__((packed));

struct ahci_t {
	uint16_t seg;
	uint8_t bus;
	uint8_t dev;
	uint8_t func;
	uint8_t class_code;
	uint8_t subclass;
	uint8_t prog_if;
	uint8_t rev_id;
	uint8_t num_com_slots;
	uint8_t lock;
	uint64_t hba_reg;
	uint32_t used_com;
	struct ahci_command_table_t* com_tables_v[32];
	uint32_t com_tables_p[32];
	struct {
		struct ahci_command_header_t* com_list;
		struct ahci_recv_fis_t* recv_fis;
		uint8_t lock;
	}* ports[32];
};

struct ahci_disk_t {
	struct ahci_t* ahci;
	uint32_t port;
};

static inline void hba_write(struct ahci_t* ahci, uint64_t off, uint32_t val) {
	*(volatile uint32_t*)(ahci->hba_reg + off) = val;
}

static inline uint32_t hba_read(struct ahci_t* ahci, uint64_t off) {
	return *(volatile uint32_t*)(ahci->hba_reg + off);
}

static inline void port_clear_errors(struct ahci_t* ahci, uint32_t port) {
	hba_write(ahci, PXSERR_OFF(port), SERR_CLEAR);
	hba_write(ahci, PXIS_OFF(port), ~0u);
}

static uint8_t find_slot(struct ahci_t* ahci, uint32_t port) {
	uint8_t slot;
	uint32_t read;

	read = hba_read(ahci, PXCI_OFF(port));

	for (slot = 0; slot < ahci->num_com_slots; slot++) {
		if (!(ahci->used_com & (1u << slot)) && !(read & (1u << slot))) {
			return slot;
		}
	}

	//TODO: assert valid slot before using
	return SLOT_NO_SLOT;
}

static enum disk_error_t ahci_read_lba(void* cntx, void* buffer, uint64_t lba, uint16_t count) {
	struct ahci_disk_t* ahci_disk = cntx;
	struct ahci_t* ahci = ahci_disk->ahci;
	uint32_t port = ahci_disk->port;
	uint8_t slot;
	enum disk_error_t error;

	uint64_t vaddr_buf, i;
	uint32_t paddr_buf;

	const uint64_t size = (uint64_t)count * SECTOR_SIZE;

	if (size > PAGE_SIZE_2M * 2) {
		return DISK_ERROR;
	}

	paddr_buf = (uint32_t)mm_alloc_pmax(size, 0, ~0u);
	if (!paddr_buf) {
		logging_log_error("Failed to allocate memory for AHCI read buffer");
		return DISK_ERROR;
	}

	vaddr_buf = mm_alloc_v(size);
	if (!vaddr_buf) {
		mm_free_p(paddr_buf, size);

		logging_log_error("Failed to allocate memory for AHCI read buffer");
		return DISK_ERROR;
	}

	for (i = 0; i < size; i += PAGE_SIZE_4K) {
		paging_map(vaddr_buf + i, paddr_buf + i, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
	}

	lock_acquire(&ahci->lock);
	slot = find_slot(ahci, port);
	ahci->used_com |= 1u << slot;
	lock_release(&ahci->lock);

	kmemset(&ahci->ports[port]->com_list[slot], 0, sizeof(struct ahci_command_header_t));
	kmemset(ahci->com_tables_v[slot], 0, PAGE_SIZE_4K);

	ahci->com_tables_v[slot]->cfis.h2d.fis_type = SATA_FIS_TYPE_H2D;
	ahci->com_tables_v[slot]->cfis.h2d.flag = SATA_FIS_H2D_C;
	ahci->com_tables_v[slot]->cfis.h2d.cmd = SATA_FIS_CMD_DMA_READ_EXT;
	ahci->com_tables_v[slot]->cfis.h2d.lba0 = (lba >> 0) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.lba1 = (lba >> 8) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.lba2 = (lba >> 16) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.lba3 = (lba >> 24) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.lba4 = (lba >> 32) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.lba5 = (lba >> 40) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.dev = 1u << 6;
	ahci->com_tables_v[slot]->cfis.h2d.count_lo = (count) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.count_hi = (count >> 8) & 0xFF;
	
	ahci->com_tables_v[slot]->prdt[0].dba = paddr_buf;
	ahci->com_tables_v[slot]->prdt[0].dbau = 0;
	ahci->com_tables_v[slot]->prdt[0].dbc_i = (uint32_t)size - 1;

	lock_acquire(&ahci->ports[port]->lock);

	port_clear_errors(ahci, port);

	kmemset(ahci->ports[port]->recv_fis, 0, sizeof(struct ahci_recv_fis_t));

	ahci->ports[port]->com_list[slot].flg = 5;
	ahci->ports[port]->com_list[slot].prdtl = 1;
	ahci->ports[port]->com_list[slot].ctba0 = ahci->com_tables_p[slot];
	ahci->ports[port]->com_list[slot].ctba_u0 = 0;


	while (hba_read(ahci, PXTFD_OFF(port)) & TFD_STS_CON_MASK) {
		time_busy_wait(10 * TIME_CONV_MS_TO_NS);
	}

	hba_write(ahci, PXCI_OFF(port), 1u << slot);
	
	while (hba_read(ahci, PXCI_OFF(port)) & (1u << slot)) {
		time_busy_wait(10 * TIME_CONV_MS_TO_NS);

		if (hba_read(ahci, PXIS_OFF(port)) & IS_TFES) {
			logging_log_error("AHCI error while reading");
			error = DISK_ERROR;
			lock_release(&ahci->ports[port]->lock);

			lock_acquire(&ahci->lock);
			ahci->used_com &= ~(1u << slot);
			lock_release(&ahci->lock);
			goto cleanup;
		}
	}

	if (hba_read(ahci, PXIS_OFF(port)) & IS_TFES) {
		logging_log_error("AHCI error while reading");
		error = DISK_ERROR;

		lock_release(&ahci->ports[port]->lock);

		lock_acquire(&ahci->lock);
		ahci->used_com &= ~(1u << slot);
		lock_release(&ahci->lock);
		goto cleanup;
	}

	lock_release(&ahci->ports[port]->lock);

	lock_acquire(&ahci->lock);
	ahci->used_com &= ~(1u << slot);
	lock_release(&ahci->lock);

	kmemcpy(buffer, (void*)vaddr_buf, size);

	error = DISK_OK;
cleanup:
	for (i = 0; i < size; i += PAGE_SIZE_4K) {
		paging_unmap(vaddr_buf + i, PAGE_4K);
	}

	mm_free_v(vaddr_buf, size);
	mm_free_p(paddr_buf, size);

	return error;
}

static enum disk_error_t ahci_write_lba(void* cntx, void* buffer, uint64_t lba, uint16_t count) {
	struct ahci_disk_t* ahci_disk = cntx;
	struct ahci_t* ahci = ahci_disk->ahci;
	uint32_t port = ahci_disk->port;
	uint8_t slot;
	enum disk_error_t error;

	uint64_t vaddr_buf, i;
	uint32_t paddr_buf;

	const uint64_t size = (uint64_t)count * SECTOR_SIZE;

	if (size > PAGE_SIZE_2M * 2) {
		return DISK_ERROR;
	}

	paddr_buf = (uint32_t)mm_alloc_pmax(size, 0, ~0u);
	if (!paddr_buf) {
		logging_log_error("Failed to allocate memory for AHCI read buffer");
		return DISK_ERROR;
	}

	vaddr_buf = mm_alloc_v(size);
	if (!vaddr_buf) {
		mm_free_p(paddr_buf, size);

		logging_log_error("Failed to allocate memory for AHCI read buffer");
		return DISK_ERROR;
	}

	for (i = 0; i < size; i += PAGE_SIZE_4K) {
		paging_map(vaddr_buf + i, paddr_buf + i, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
	}

	kmemcpy((void*)vaddr_buf, buffer, size);

	lock_acquire(&ahci->lock);
	slot = find_slot(ahci, port);
	ahci->used_com |= 1u << slot;
	lock_release(&ahci->lock);

	kmemset(&ahci->ports[port]->com_list[slot], 0, sizeof(struct ahci_command_header_t));
	kmemset(ahci->com_tables_v[slot], 0, PAGE_SIZE_4K);

	ahci->com_tables_v[slot]->cfis.h2d.fis_type = SATA_FIS_TYPE_H2D;
	ahci->com_tables_v[slot]->cfis.h2d.flag = SATA_FIS_H2D_C;
	ahci->com_tables_v[slot]->cfis.h2d.cmd = SATA_FIS_CMD_DMA_WRITE_EXT;
	ahci->com_tables_v[slot]->cfis.h2d.lba0 = (lba >> 0) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.lba1 = (lba >> 8) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.lba2 = (lba >> 16) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.lba3 = (lba >> 24) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.lba4 = (lba >> 32) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.lba5 = (lba >> 40) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.dev = 1u << 6;
	ahci->com_tables_v[slot]->cfis.h2d.count_lo = (count) & 0xFF;
	ahci->com_tables_v[slot]->cfis.h2d.count_hi = (count >> 8) & 0xFF;
	
	ahci->com_tables_v[slot]->prdt[0].dba = paddr_buf;
	ahci->com_tables_v[slot]->prdt[0].dbau = 0;
	ahci->com_tables_v[slot]->prdt[0].dbc_i = (uint32_t)size - 1;

	lock_acquire(&ahci->ports[port]->lock);

	port_clear_errors(ahci, port);

	kmemset(ahci->ports[port]->recv_fis, 0, sizeof(struct ahci_recv_fis_t));

	ahci->ports[port]->com_list[slot].flg = 5 | SATA_FIS_CMD_W;
	ahci->ports[port]->com_list[slot].prdtl = 1;
	ahci->ports[port]->com_list[slot].ctba0 = ahci->com_tables_p[slot];
	ahci->ports[port]->com_list[slot].ctba_u0 = 0;


	while (hba_read(ahci, PXTFD_OFF(port)) & TFD_STS_CON_MASK) {
		time_busy_wait(10 * TIME_CONV_MS_TO_NS);
	}

	hba_write(ahci, PXCI_OFF(port), 1u << slot);
	
	while (hba_read(ahci, PXCI_OFF(port)) & (1u << slot)) {
		time_busy_wait(10 * TIME_CONV_MS_TO_NS);

		if (hba_read(ahci, PXIS_OFF(port)) & IS_TFES) {
			logging_log_error("AHCI error while reading");
			error = DISK_ERROR;
			lock_release(&ahci->ports[port]->lock);

			lock_acquire(&ahci->lock);
			ahci->used_com &= ~(1u << slot);
			lock_release(&ahci->lock);
			goto cleanup;
		}
	}

	if (hba_read(ahci, PXIS_OFF(port)) & IS_TFES) {
		logging_log_error("AHCI error while reading");
		error = DISK_ERROR;

		lock_release(&ahci->ports[port]->lock);

		lock_acquire(&ahci->lock);
		ahci->used_com &= ~(1u << slot);
		lock_release(&ahci->lock);
		goto cleanup;
	}

	lock_release(&ahci->ports[port]->lock);

	lock_acquire(&ahci->lock);
	ahci->used_com &= ~(1u << slot);
	lock_release(&ahci->lock);

	error = DISK_OK;
cleanup:
	for (i = 0; i < size; i += PAGE_SIZE_4K) {
		paging_unmap(vaddr_buf + i, PAGE_4K);
	}

	mm_free_v(vaddr_buf, size);
	mm_free_p(paddr_buf, size);

	return error;
}

static enum disk_error_t ahci_flush_cache(void* cntx) {
	struct ahci_disk_t* ahci_disk = cntx;
	struct ahci_t* ahci = ahci_disk->ahci;
	uint32_t port = ahci_disk->port;
	uint8_t slot;


	lock_acquire(&ahci->lock);
	slot = find_slot(ahci, port);
	ahci->used_com |= 1u << slot;
	lock_release(&ahci->lock);

	kmemset(&ahci->ports[port]->com_list[slot], 0, sizeof(struct ahci_command_header_t));
	kmemset(ahci->com_tables_v[slot], 0, PAGE_SIZE_4K);

	ahci->com_tables_v[slot]->cfis.h2d.fis_type = SATA_FIS_TYPE_H2D;
	ahci->com_tables_v[slot]->cfis.h2d.flag = SATA_FIS_H2D_C;
	ahci->com_tables_v[slot]->cfis.h2d.cmd = SATA_FIS_CMD_FLUSH_CACHE_EXT;

	lock_acquire(&ahci->ports[port]->lock);

	port_clear_errors(ahci, port);

	kmemset(ahci->ports[port]->recv_fis, 0, sizeof(struct ahci_recv_fis_t));

	ahci->ports[port]->com_list[slot].flg   = 5;
	ahci->ports[port]->com_list[slot].prdtl = 0;
	ahci->ports[port]->com_list[slot].ctba0 = ahci->com_tables_p[slot];
	ahci->ports[port]->com_list[slot].ctba_u0 = 0;

	while (hba_read(ahci, PXTFD_OFF(port)) & TFD_STS_CON_MASK) {
		time_busy_wait(10 * TIME_CONV_MS_TO_NS);
	}

	hba_write(ahci, PXCI_OFF(port), 1u << slot);

	while (hba_read(ahci, PXCI_OFF(port)) & (1u << slot)) {
		time_busy_wait(10 * TIME_CONV_MS_TO_NS);

		if (hba_read(ahci, PXIS_OFF(port)) & IS_TFES) {
			logging_log_error("AHCI flush cache error");
			lock_release(&ahci->ports[port]->lock);

			lock_acquire(&ahci->lock);
			ahci->used_com &= ~(1u << slot);
			lock_release(&ahci->lock);

			return DISK_ERROR;
		}
	}

	lock_release(&ahci->ports[port]->lock);
	
	lock_acquire(&ahci->lock);
	ahci->used_com &= ~(1u << slot);
	lock_release(&ahci->lock);
	
	return DISK_OK;
}

static void port_identify(struct ahci_t* ahci, uint32_t port) {
	uint8_t slot;
	uint32_t paddr_identity;
	volatile uint16_t* identity;
	uint8_t model[41];
	uint64_t lba48;

	paddr_identity = (uint32_t)mm_alloc_pmax(PAGE_SIZE_4K, 0, ~0u);
	if (!paddr_identity) {
		logging_log_error("Failed to allocate memory for AHCI identity buffer");
		return;
	}

	identity = (uint16_t*)mm_alloc_v(PAGE_SIZE_4K);
	if (!identity) {
		mm_free_p(paddr_identity, PAGE_SIZE_4K);

		logging_log_error("Failed to allocate memory for AHCI identity buffer");
		return;
	}

	paging_map((uint64_t)identity, paddr_identity, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);

	lock_acquire(&ahci->lock);
	slot = find_slot(ahci, port);
	ahci->used_com |= 1u << slot;
	lock_release(&ahci->lock);

	kmemset(&ahci->ports[port]->com_list[slot], 0, sizeof(struct ahci_command_header_t));
	kmemset(ahci->com_tables_v[slot], 0, PAGE_SIZE_4K);
	kmemset((void*)identity, 0, PAGE_SIZE_4K);

	ahci->com_tables_v[slot]->prdt[0].dba = paddr_identity;
	ahci->com_tables_v[slot]->prdt[0].dbau = 0;
	ahci->com_tables_v[slot]->prdt[0].dbc_i = 512 - 1;

	ahci->com_tables_v[slot]->cfis.h2d.fis_type = SATA_FIS_TYPE_H2D;
	ahci->com_tables_v[slot]->cfis.h2d.flag = SATA_FIS_H2D_C;
	ahci->com_tables_v[slot]->cfis.h2d.cmd = SATA_FIS_CMD_IDENT;

	lock_acquire(&ahci->ports[port]->lock);

	port_clear_errors(ahci, port);

	kmemset(ahci->ports[port]->recv_fis, 0, sizeof(struct ahci_recv_fis_t));

	ahci->ports[port]->com_list[slot].flg = 5;
	ahci->ports[port]->com_list[slot].prdtl = 1;
	ahci->ports[port]->com_list[slot].ctba0 = ahci->com_tables_p[slot];
	ahci->ports[port]->com_list[slot].ctba_u0 = 0;

	while (hba_read(ahci, PXTFD_OFF(port)) & TFD_STS_CON_MASK) {
		time_busy_wait(10 * TIME_CONV_MS_TO_NS);
	}

	hba_write(ahci, PXCI_OFF(port), 1u << slot);
	
	while (hba_read(ahci, PXCI_OFF(port)) & (1u << slot)) {
		time_busy_wait(10 * TIME_CONV_MS_TO_NS);
	}

	lock_release(&ahci->ports[port]->lock);

	lock_acquire(&ahci->lock);
	ahci->used_com &= ~(1u << slot);
	lock_release(&ahci->lock);

	for (int i = 0; i < 20; i++) {
			model[i*2]   = identity[27 + i] >> 8;
			model[i*2+1] = identity[27 + i] & 0xFF;
	}
	model[40] = 0;

	lba48 = *(uint64_t*)&identity[100];

	logging_log_debug("Found ATA drive %s 0x%lx", &model[0], lba48);

	paging_unmap((uint64_t)identity, PAGE_4K);

	mm_free_v((uint64_t)identity, PAGE_SIZE_4K);

	mm_free_p(paddr_identity, PAGE_SIZE_4K);
}

static void port_reset(struct ahci_t* ahci, uint32_t i) {
	uint32_t read;

	read = hba_read(ahci, PXCMD_OFF(i));
	hba_write(ahci, PXCMD_OFF(i), read & ~CMD_ST);
	time_busy_wait(10 * TIME_CONV_MS_TO_NS);
	read = hba_read(ahci, PXCMD_OFF(i));
	if (read & CMD_CR) {
		time_busy_wait(490 * TIME_CONV_MS_TO_NS);
		read = hba_read(ahci, PXCMD_OFF(i));
		if (read & CMD_CR) {
			logging_log_error("AHCI port %u hung", i);
		}
	}

	read = hba_read(ahci, PXSCTL_OFF(i));
	read &= ~SCTL_DET_MSK;
	read |= SCTL_DET_RST;
	hba_write(ahci, PXSCTL_OFF(i), read);

	time_busy_wait(1 * TIME_CONV_MS_TO_NS);
	read = hba_read(ahci, PXSCTL_OFF(i));
	read &= ~SCTL_DET_MSK;
	hba_write(ahci, PXSCTL_OFF(i), read);

	do {
		time_busy_wait(10 * TIME_CONV_MS_TO_NS);
		read = hba_read(ahci, PXSSTS_OFF(i));
	} while ((read & SSTS_DET_MASK) != SSTS_DET_EST);

	port_clear_errors(ahci, i);
	logging_log_debug("HBA port %u reset complete", i);
}

static void ahci_init(void* cntx) {
	struct ahci_t* ahci = cntx;
	uint32_t bar, i, read, ports, j;
	uint8_t s64a;
	uint32_t cl_pool_p = 0;
	uint32_t fis_pool_p = 0;
	uint64_t cl_pool_v = 0;
	uint64_t fis_pool_v = 0;
	struct ahci_disk_t* ahci_disk;

	logging_log_info("AHCI driver initialization for %u/%u/%u/%u at %u.%u.%u.%u",
			ahci->class_code, ahci->subclass, ahci->prog_if, ahci->rev_id, ahci->seg, ahci->bus, ahci->dev, ahci->func);

	lock_init(&ahci->lock);

	ahci->used_com = 0;

	bar = pcie_read(ahci->seg, ahci->bus, ahci->dev, ahci->func, PCI_BAR5_REG) & PCI_BAR_BA_MAKS;
	if (!bar) {
		bar = (uint32_t)mm_alloc_pmax(0x2000, 0x2000, (uint64_t)~((uint32_t)0));
		if (!bar) {
			logging_log_error("Failed to allocate memory for AHCI HBA space");
			panic(PANIC_NO_MEM);
		}
	}
	pcie_write(ahci->seg, ahci->bus, ahci->dev, ahci->func, PCI_BAR5_REG, bar);

	//TODO: support BIOS/OS handoff

	ahci->hba_reg = mm_alloc_v(0x2000);
	if (!ahci->hba_reg) {
		logging_log_error("Failed to allocate for HBA register space");
		panic(PANIC_NO_MEM);
	}

	paging_map(ahci->hba_reg, bar, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
	paging_map(ahci->hba_reg + PAGE_SIZE_4K, bar + PAGE_SIZE_4K, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);

	logging_log_debug("AHCI HBA mapped to 0x%lx", ahci->hba_reg);

	for (i = 0; i < 32; i++) {
		ahci->com_tables_p[i] = (uint32_t)mm_alloc_pmax(PAGE_SIZE_4K, 0, ~0u);

		if (!ahci->com_tables_p[i]) {
			logging_log_error("Failed to allocate AHCI command table");
			panic(PANIC_NO_MEM);
		}

		ahci->com_tables_v[i] = (struct ahci_command_table_t*)mm_alloc_v(PAGE_SIZE_4K);
		if (!ahci->com_tables_v[i]) {
			logging_log_error("Failed to allocate AHCI command table");
			panic(PANIC_NO_MEM);
		}

		paging_map((uint64_t)ahci->com_tables_v[i], ahci->com_tables_p[i],
				PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
	}

	/* minimal initialization sequence */
	read = hba_read(ahci, GHC_OFF);
	read &= ~GHC_IE;
	read |= GHC_AE;
	hba_write(ahci, GHC_OFF, read);

	ports = hba_read(ahci, PI_OFF);

	for (i = 0; i < 32; i++) {
		if (ports & (1u << i)) {
			logging_log_debug("AHCI supports port %u", i);

			read = hba_read(ahci, PXCMD_OFF(i));
			if (read & (CMD_ST | CMD_FRE | CMD_FR | CMD_CR)) {
				/* stop port execution */
				hba_write(ahci, PXCMD_OFF(i), read & ~CMD_ST);
				time_busy_wait(10 * TIME_CONV_MS_TO_NS);
				read = hba_read(ahci, PXCMD_OFF(i));
				if (read & CMD_CR) {
					time_busy_wait(490 * TIME_CONV_MS_TO_NS);
					read = hba_read(ahci, PXCMD_OFF(i));
					if (read & CMD_CR) {
						logging_log_error("Failed to stop AHCI port %u. Port reset", i);
						port_reset(ahci, i);
						i--;
						continue;
					}
				}

				if (read & CMD_FRE) {
					hba_write(ahci, PXCMD_OFF(i), read & !CMD_FRE);
					time_busy_wait(10 * TIME_CONV_MS_TO_NS);
					read = hba_read(ahci, PXCMD_OFF(i));
					if (read & CMD_FR) {
						time_busy_wait(490 * TIME_CONV_MS_TO_NS);
						read = hba_read(ahci, PXCMD_OFF(i));
						if (read & CMD_FR) {
							logging_log_error("Failed to stop AHCI port %u. Port reset", i);
							port_reset(ahci, i);
							i--;
							continue;
						}
					}
				}
			}
		}
	}

	read = hba_read(ahci, CAP_OFF);
	ahci->num_com_slots = (read & CAP_NCS_MASK) >> CAP_NCS_SHFT;
	s64a = !!(read & CAP_S64A);

	j = 0;
	for (i = 0; i < 32; i++) {
		if (ports & (1u << i)) {
			ahci->ports[i] = kmalloc(sizeof(*ahci->ports[i]));

			lock_init(&ahci->ports[i]->lock);

			if (j % (PAGE_SIZE_4K / CL_SIZE) == 0) {
				cl_pool_p = (uint32_t)mm_alloc_pmax(PAGE_SIZE_4K, 0, ~0u);
				if (!cl_pool_p) {
					logging_log_error("Failed to allocate command list buffer");
					panic(PANIC_NO_MEM);
				}

				cl_pool_v = mm_alloc_v(PAGE_SIZE_4K);
				if (!cl_pool_v) {
					logging_log_error("Failed to allocate command list buffer");
					panic(PANIC_NO_MEM);
				}

				paging_map(cl_pool_v, cl_pool_p, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
			}

			if (j % (PAGE_SIZE_4K / FIS_SIZE) == 0) {
				fis_pool_p = (uint32_t)mm_alloc_pmax(PAGE_SIZE_4K, 0, ~0u);
				if (!fis_pool_p) {
					logging_log_error("Failed to allocate FIS buffer");
					panic(PANIC_NO_MEM);
				}

				fis_pool_v = mm_alloc_v(PAGE_SIZE_4K);
				if (!fis_pool_v) {
					logging_log_error("Failed to allocate FIS list buffer");
					panic(PANIC_NO_MEM);
				}

				paging_map(fis_pool_v, fis_pool_p, PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);
			}

			hba_write(ahci, PXCLB_OFF(i), cl_pool_p);
			ahci->ports[i]->com_list = (struct ahci_command_header_t*)cl_pool_v;

			hba_write(ahci, PXFB_OFF(i), fis_pool_p);
			ahci->ports[i]->recv_fis = (struct ahci_recv_fis_t*)fis_pool_v;

			kmemset((void*)cl_pool_v, 0, CL_SIZE);
			kmemset((void*)fis_pool_v, 0, FIS_SIZE);

			cl_pool_p += CL_SIZE;
			cl_pool_v += CL_SIZE;

			fis_pool_p += FIS_SIZE;
			fis_pool_v += FIS_SIZE;
			j++;

			if (s64a) {
				hba_write(ahci, PXCLBU_OFF(i), 0);
				hba_write(ahci, PXFBU_OFF(i), 0);
			}

			read = hba_read(ahci, PXCMD_OFF(i));
			read |= CMD_FRE;
			hba_write(ahci, PXCMD_OFF(i), read);
			port_clear_errors(ahci, i); // clears X

			logging_log_debug("AHCI port %u ready", i);

			/* detect for connected device */
			read = hba_read(ahci, PXTFD_OFF(i));
			if (read & TFD_STS_CON_MASK) {
				continue;
			}

			read = hba_read(ahci, PXSSTS_OFF(i));
			if ((read & SSTS_DET_MASK) != SSTS_DET_EST) {
				continue;
			}

			read = hba_read(ahci, PXCMD_OFF(i));
			read |= CMD_ST;
			hba_write(ahci, PXCMD_OFF(i), read);

			port_identify(ahci, i);
			ahci_disk = kmalloc(sizeof(struct ahci_disk_t));
			ahci_disk->ahci = ahci;
			ahci_disk->port = i;
			disk_add(ahci_disk, ahci_read_lba, ahci_write_lba, ahci_flush_cache);
		}
		else {
			ahci->ports[i] = 0;
		}
	}

	logging_log_info("AHCI init done");
}

void ahci_generic(
		uint16_t seg,
		uint8_t bus,
		uint8_t dev,
		uint8_t func,
		uint8_t class_code,
		uint8_t subclass,
		uint8_t prog_if,
		uint8_t rev_id) {

	struct ahci_t* ahci = kmalloc(sizeof(struct ahci_t));
	ahci->seg = seg;
	ahci->bus = bus;
	ahci->dev = dev;
	ahci->func = func;
	ahci->class_code = class_code;
	ahci->subclass = subclass;
	ahci->prog_if = prog_if;
	ahci->rev_id = rev_id;

	scheduler_schedule(process_from_func(ahci_init, ahci));
}
