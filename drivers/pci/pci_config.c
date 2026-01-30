/* pci_config.c - Peripheral Controller Interface configuration space access */
/* Copyright (C) 2025  Ebrahim Aleem
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

#include <pci/pci_config.h>

#include <kernel/core/ports.h>

#define CONF_ADDR	0xCF8
#define CONF_DATA 0xCFC

#define REG_MASK	0xFC
#define REG_SIZE	4

union conf_addr_t {
	uint32_t raw;
	struct {
		uint8_t reg;
		uint8_t fun;
		uint8_t dev;
		uint8_t bus;
		uint8_t flg;
	} conf_addr __attribute__((packed));
};

uint32_t pci_read_conf(uint8_t reg, uint8_t fun, uint8_t dev, uint8_t bus) {
	outd(CONF_ADDR, (union conf_addr_t){
			.conf_addr.reg = reg,
			.conf_addr.fun = fun,
			.conf_addr.dev = dev,
			.conf_addr.bus = bus
			}.raw
		);

	io_wait();

	return ind(CONF_DATA);
}

void pci_write_conf(uint8_t reg, uint8_t fun, uint8_t dev, uint8_t bus, uint32_t data) {
	outd(CONF_ADDR, (union conf_addr_t){
			.conf_addr.reg = reg,
			.conf_addr.fun = fun,
			.conf_addr.dev = dev,
			.conf_addr.bus = bus
			}.raw
		);

	io_wait();

	outd(CONF_DATA, data);
}

uint64_t pci_read_conf_noalign(uint8_t reg, uint8_t fun, uint8_t dev, uint8_t bus, uint8_t width) {
	const uint8_t reg_base = reg & REG_MASK;
	uint32_t data0;

	switch (reg - reg_base) {
		case 0:
			data0 = pci_read_conf(reg_base, fun, dev, bus);
			switch (width) {
				case 8:
					return data0 & 0xFF;
				case 16:
					return data0 & 0xFFFF;
				case 32:
					return data0;
				case 64:
					return (uint64_t)data0 
						| ((uint64_t)pci_read_conf(reg_base + REG_SIZE, fun, dev, bus) << 32);
				default:
					return 0;
			}
		case 1:
			data0 = pci_read_conf(reg_base, fun, dev, bus) >> 8;
			switch (width) {
				case 8:
					return data0 & 0xFF;
				case 16:
					return data0 & 0xFFFF;
				case 32:
					return (uint32_t)data0 
						| (((uint32_t)pci_read_conf(reg_base + REG_SIZE, fun, dev, bus) & 0xFF) << 24);
				case 64:
					return (uint64_t)data0 
						| (((uint64_t)pci_read_conf(reg_base + REG_SIZE, fun, dev, bus)) << 24)
						| (((uint64_t)pci_read_conf(reg_base + REG_SIZE * 2, fun, dev, bus) & 0xFF) << 56);
				default:
					return 0;
			}
		case 2:
			data0 = pci_read_conf(reg_base, fun, dev, bus) >> 16;
			switch (width) {
				case 8:
					return data0 & 0xFF;
				case 16:
					return data0;
				case 32:
					return (uint32_t)data0
						| (((uint32_t)pci_read_conf(reg_base + REG_SIZE, fun, dev, bus) & 0xFFFF) << 16);
				case 64:
					return (uint64_t)data0
						| (((uint64_t)pci_read_conf(reg_base + REG_SIZE, fun, dev, bus)) << 16)
						| (((uint64_t)pci_read_conf(reg_base + REG_SIZE * 2, fun, dev, bus) & 0xFFFF) << 48);
				default:
					return 0;
			}
		case 3:
			data0 = pci_read_conf(reg_base, fun, dev, bus) >> 24;
			switch (width) {
				case 8:
					return data0;
				case 16:
					return (uint32_t)data0
						| (((uint32_t)pci_read_conf(reg_base + REG_SIZE, fun, dev, bus) & 0xFF) << 8);
				case 32:
					return (uint32_t)data0
						| (((uint32_t)pci_read_conf(reg_base + REG_SIZE, fun, dev, bus) & 0xFFFFFF) << 8);
				case 64:
					return (uint64_t)data0
						| (((uint64_t)pci_read_conf(reg_base + REG_SIZE, fun, dev, bus)) << 8)
						| (((uint64_t)pci_read_conf(reg_base + REG_SIZE * 2, fun, dev, bus) & 0xFFFFFF) << 40);
				default:
					return 0;
			}
		default:
			return 0;
	}
}

void pci_write_conf_noalign(uint8_t reg, uint8_t fun, uint8_t dev, uint8_t bus, uint8_t width, uint64_t data) {
	const uint8_t reg_base = reg & REG_MASK;

	switch (reg - reg_base) {
		case 0:
			switch (width) {
				case 8:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFFFFFF00) | (data & 0xFF));
					return;
				case 16:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFFFF0000) | (data & 0xFFFF));
					return;
				case 32:
					pci_write_conf(reg_base, fun, dev, bus, data & 0xFFFFFFFF);
					return;
				case 64:
					pci_write_conf(reg_base, fun, dev, bus, data & 0xFFFFFFFF);
					pci_write_conf(reg_base + REG_SIZE, fun, dev, bus, (data >> 32) & 0xFFFFFFFF);
					return;
				default:
					return;
			}
		case 1:
			switch (width) {
				case 8:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFFFF00FF) | ((data << 8) & 0xFF00));
					return;
				case 16:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFF0000FF) | ((data << 8) & 0xFFFF00));
					return;
				case 32:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFF) | ((data << 8) & 0xFFFFFF00));
					pci_write_conf(reg_base + REG_SIZE, fun, dev, bus,
							(pci_read_conf(reg_base + REG_SIZE, fun, dev, bus) & 0xFFFFFF00) | (data & 0xFF));
					return;
				case 64:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFF) | ((data << 8) & 0xFFFFFF00));
					pci_write_conf(reg_base + REG_SIZE, fun, dev, bus, ((data >> 24) & 0xFFFFFFFF));
					pci_write_conf(reg_base + REG_SIZE * 2, fun, dev, bus,
							(pci_read_conf(reg_base + REG_SIZE * 2, fun, dev, bus) & 0xFFFFFF00) | ((data >> 56) & 0xFF));
					return;
				default:
					return;
			}
		case 2:
			switch (width) {
				case 8:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFF00FFFF) | ((data << 16) & 0xFF0000));
					return;
				case 16:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFFFF) | ((data << 16) & 0xFFFF0000));
					return;
				case 32:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFFFF) | ((data << 16) & 0xFFFF0000));
					pci_write_conf(reg_base + REG_SIZE, fun, dev, bus,
							(pci_read_conf(reg_base + REG_SIZE, fun, dev, bus) & 0xFFFF0000) | ((data >> 16) & 0xFFFF));
					return;
				case 64:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFFFF) | ((data << 16) & 0xFFFF0000));
					pci_write_conf(reg_base + REG_SIZE, fun, dev, bus,
							((data >> 16) & 0xFFFFFFFF));
					pci_write_conf(reg_base + REG_SIZE * 2, fun, dev, bus,
							(pci_read_conf(reg_base + REG_SIZE * 2, fun, dev, bus) & 0xFFFF0000) | ((data >> 48) & 0xFFFF));
					return;
				default:
					return;
			}
		case 3:
			switch (width) {
				case 8:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFFFFFF) | ((data << 24) & 0xFF000000));
					return;
				case 16:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFFFFFF) | ((data << 24) & 0xFF000000));
					pci_write_conf(reg_base + REG_SIZE, fun, dev, bus,
							(pci_read_conf(reg_base + REG_SIZE, fun, dev, bus) & 0xFFFFFF00) | ((data >> 8) & 0xFF));
					return;
				case 32:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFFFFFF) | ((data << 24) & 0xFF000000));
					pci_write_conf(reg_base + REG_SIZE, fun, dev, bus,
							(pci_read_conf(reg_base + REG_SIZE, fun, dev, bus) & 0xFF000000) | ((data >> 8) & 0xFFFFFF));
					return;
				case 64:
					pci_write_conf(reg_base, fun, dev, bus,
							(pci_read_conf(reg_base, fun, dev, bus) & 0xFFFFFF) | ((data << 24) & 0xFF000000));
					pci_write_conf(reg_base + REG_SIZE, fun, dev, bus, (data >> 8) & 0xFFFFFFFF);
					pci_write_conf(reg_base + REG_SIZE * 2, fun, dev, bus,
							(pci_read_conf(reg_base + REG_SIZE * 2, fun, dev, bus) & 0xFF000000) | ((data >> 40) & 0xFFFFFF));
					return;
				default:
					return;
			}
		default:
			return;
	}
}
