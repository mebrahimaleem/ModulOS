/* tables.c - ACPI tables */
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

#include <acpi/tables.h>

#include <kernel/core/kentry.h>
#include <kernel/core/panic.h>
#include <kernel/core/logging.h>
#include <kernel/core/paging.h>
#include <kernel/core/alloc.h>

#include <kernel/lib/memcmp.h>
#include <kernel/lib/hash.h>

#define V1_TABLE_SIZE	20

#define RSDPV1	1
#define RSDPV2	2

#define MADT_CONTROLLER_IO_APIC	1

#define FOUND_FADT	0x01
#define FOUND_MADT	0x02
#define FOUND_HPET	0x04

struct acpi_gen_header_t {
	uint8_t Signature[4];
	uint32_t Length;
};

struct acpi_gas_t {
	uint8_t AddressSpaceID;
	uint8_t RegisterBitWidth;
	uint8_t RegisterBitOffset;
	uint8_t AccessSize;
	uint64_t Address;	
} __attribute__((packed));

struct acpi_xsdt_t {
	uint8_t Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	uint8_t OEMID[6];
	uint8_t OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
	uint64_t Entry[];
} __attribute__((packed));

struct acpi_rsdt_t {
	uint8_t Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	uint8_t OEMID[6];
	uint8_t OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
	uint32_t Entry[];
} __attribute__((packed));

struct acpi_fadt_t {
	uint8_t Signature[4];
	uint32_t Length;
	uint8_t FADTMajorVersion;
	uint8_t Checksum;
	uint8_t OEMID[6];
	uint8_t OEMTableID[8];
	uint8_t _[];
} __attribute__((packed));

struct acpi_madt_ics_gen_t {
	uint8_t type;
	uint8_t len;
	uint8_t _[];
} __attribute__((packed));

struct acpi_madt_ics_io_apic_t {
	uint8_t type;
	uint8_t len;
	uint8_t IOAPICID;
	uint8_t Reserved;
	uint32_t IOAPICAddress;
	uint32_t GlobalSystemInterruptBase;
} __attribute__((packed));

struct acpi_madt_t {
	uint8_t Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	uint8_t OEMID[6];
	uint8_t OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
	uint32_t LocalInterruptControllerAddress;
	uint32_t Flags;
	uint8_t InterruptControllerStructure[];
} __attribute__((packed));

struct acpi_hpet_t {
	uint8_t Signature[4];
	uint32_t Length;
	uint8_t Revision;
	uint8_t Checksum;
	uint8_t OEMID[6];
	uint8_t OEMTableID[8];
	uint32_t OEMRevision;
	uint32_t CreatorID;
	uint32_t CreatorRevision;
	struct {
		uint8_t HardwareRevID;
		uint8_t NumComparator_CounterSizer_Legacy;
		uint16_t PCIVendorID;
	} __attribute__((packed)) EventTimerBlockID;
	struct acpi_gas_t BASE_ADDRESS;
	uint8_t HPETNumber;
	uint16_t MainCounterMinimumClockTick;
	uint8_t PageProtectionAndOEMAttribute;
} __attribute__((packed));

static struct {
	struct acpi_io_apic_list_t* io_apics;
	struct acpi_hpet_base_t hpet;
} acpi_index;

static void map_header(const volatile void* table) {
	// ACPI memory will never collide, so identity map everything
	const volatile struct acpi_gen_header_t* gen = (struct acpi_gen_header_t*)table;
	uint64_t alloc_base = (uint64_t)gen & PAGE_BASE_MASK;

	paging_map(alloc_base, alloc_base, PAGE_PRESENT | PAT_MMIO_4K, PAGE_4K);

	if ((uint64_t)gen != alloc_base) {
		alloc_base += PAGE_SIZE_4K;
		paging_map(alloc_base, alloc_base, PAGE_PRESENT | PAT_MMIO_4K, PAGE_4K);
	}
}

static void map_table(const volatile void* table) {
	const volatile struct acpi_gen_header_t* gen = (struct acpi_gen_header_t*)table;
	uint64_t alloc_base = (uint64_t)gen & PAGE_BASE_MASK;

	if ((uint64_t)gen != alloc_base) {
		alloc_base += PAGE_SIZE_4K;
	}

	while (alloc_base < (uint64_t)gen + gen->Length) {
		alloc_base += PAGE_SIZE_4K;
		paging_map(alloc_base, alloc_base, PAGE_PRESENT | PAT_MMIO_4K, PAGE_4K);
	}
}

static inline uint8_t verify_checksum(const volatile void* table) {
	const volatile struct acpi_gen_header_t* gen = (struct acpi_gen_header_t*)table;
	return hash_byte_sum((void*)gen, gen->Length);
}

static void index_io_apics(const volatile struct acpi_madt_t* madt) {
	const volatile struct acpi_madt_ics_io_apic_t* io_apic;
	struct acpi_io_apic_list_t* t;

	for (const volatile struct acpi_madt_ics_gen_t* gen = 
				(const volatile struct acpi_madt_ics_gen_t*)&madt->InterruptControllerStructure[0];
			(uint64_t)gen < (uint64_t)madt + madt->Length;
			gen = (const volatile struct acpi_madt_ics_gen_t*)((uint64_t)gen + (uint64_t)gen->len)) {
		switch (gen->type) {
			case MADT_CONTROLLER_IO_APIC:
				io_apic = (const volatile struct acpi_madt_ics_io_apic_t*)gen;
				t = acpi_index.io_apics;
				acpi_index.io_apics = kmalloc(sizeof(struct acpi_io_apic_list_t));
				*acpi_index.io_apics = (struct acpi_io_apic_list_t) {
					.base = io_apic->IOAPICAddress,
					.gsi_base = io_apic->GlobalSystemInterruptBase,
					.id = io_apic->IOAPICID,
					.next = t
				};
				logging_log_info("Found IO APIC 0x%X64 @ 0x%X64 : 0x%X64 (gsi)",
						(uint64_t)io_apic->IOAPICID,
						(uint64_t)io_apic->IOAPICAddress,
						(uint64_t)io_apic->GlobalSystemInterruptBase);
				break;
			default:
				break;
		}
	}
}

void acpi_index_tables(void) {
	const volatile struct acpi_gen_header_t* gen;
	uint8_t found;

	if (memcmp(boot_context.rsdp.Signature, "RSD PTR ", 8)) {
		logging_log_error("Bad ACPI RSD PTR signature");
		panic(PANIC_ACPI);
	}

	if (hash_byte_sum(&boot_context.rsdp, V1_TABLE_SIZE)) {
		logging_log_error("Bad ACPI RSDPV1 checksum");
		panic(PANIC_ACPI);
	}

	switch (boot_context.rsdp.Revision) {
		case RSDPV1:
fallback:
			found = 0;
			const struct acpi_rsdt_t* rsdt = (const struct acpi_rsdt_t*)(uint64_t)boot_context.rsdp.RsdtAddress;
			map_header(rsdt);
			if (memcmp(rsdt->Signature, "RSDT", 4)) {
				logging_log_error("Bad ACPI RSDT signature");
				panic(PANIC_ACPI);
			}
			map_table(rsdt);

			if (verify_checksum(rsdt)) {
				logging_log_error("Bad ACPI RSDT checksum");
				panic(PANIC_ACPI);
			}

			logging_log_info("Parsing ACPI RSDT entries @ 0x%X64", (uint64_t)&rsdt->Entry[0]);

			for (const volatile uint32_t* entry = &rsdt->Entry[0];
					(uint64_t)entry < (uint64_t)rsdt + rsdt->Length; entry++) {
				gen = (const volatile struct acpi_gen_header_t*)(uint64_t)*entry;
				map_header(gen);

				//FADT
				if (!memcmp((uint8_t*)gen->Signature, "FACP", 4)) {
					map_table(gen);
					if (verify_checksum(gen)) {
						logging_log_error("Bad ACPI FACP checksum");
						panic(PANIC_ACPI);
					}

					const volatile struct acpi_fadt_t* fadt = (const volatile struct acpi_fadt_t*)gen;
					if (memcmp((uint8_t*)rsdt->OEMTableID, (uint8_t*)fadt->OEMTableID, 8)) {
						logging_log_error("Bad ACPI FACP OEM Table ID");
						panic(PANIC_ACPI);
					}

					logging_log_info("Found ACPI FACP @ 0x%X64", (uint64_t)fadt);

					found |= FOUND_FADT;
				}

				//MADT
				if (!memcmp((uint8_t*)gen->Signature, "APIC", 4)) {
					map_table(gen);
					if (verify_checksum(gen)) {
						logging_log_error("Bad ACPI MADT checksum");
						panic(PANIC_ACPI);
					}

					const volatile struct acpi_madt_t* madt = (const volatile struct acpi_madt_t*)gen;

					logging_log_info("Found ACPI MADT @ 0x%X64", (uint64_t)madt);

					index_io_apics(madt);
					found |= FOUND_MADT;
				}

				//HPET
				if (!memcmp((uint8_t*)gen->Signature, "HPET", 4)) {
					map_table(gen);
					if (verify_checksum(gen)) {
						logging_log_error("Bad ACPI HPET checksum");
						panic(PANIC_ACPI);
					}

					const volatile struct acpi_hpet_t* hpet = (const volatile struct acpi_hpet_t*)gen;

					logging_log_info("Found ACPI HPET @ 0x%X64", (uint64_t)hpet);

					acpi_index.hpet.base = (uint32_t)hpet->BASE_ADDRESS.Address; // only lower 32b are valid
					acpi_index.hpet.min_clock_tick = hpet->MainCounterMinimumClockTick;
					found |= FOUND_HPET;
				}
			}

			if (found != (FOUND_FADT | FOUND_MADT | FOUND_HPET )) {
				logging_log_error("Could not find all ACPI tables");
				panic(PANIC_ACPI);
			}
			break;
		default:
			// backwards compat, so everything else is v2
			logging_log_warning("Unkown ACPI Revision. Assuming v2");
			__attribute__((fallthrough));
		case RSDPV2:
			if (hash_byte_sum(&boot_context.rsdp, boot_context.rsdp.Length)) {
				logging_log_warning("Bad ACPI RSDPV2 checksum. Falling back to RSDPV1");
				goto fallback;
			}

			found = 0;
			const struct acpi_xsdt_t* xsdt = (const struct acpi_xsdt_t*)boot_context.rsdp.XsdtAddress;
			map_header(xsdt);
			if (memcmp(xsdt->Signature, "XSDT", 4)) {
				logging_log_warning("Bad ACPI XSDT signature. Falling back to RSDPV1");
				goto fallback;
			}
			map_table(xsdt);

			if (verify_checksum(xsdt)) {
				logging_log_warning("Bad ACPI XSDT checksum. Falling back to RSDPV1");
				goto fallback;
			}

			logging_log_info("Parsing ACPI XSDT entries @ 0x%X64", (uint64_t)&xsdt->Entry[0]);

			for (const volatile uint64_t* entry = &xsdt->Entry[0];
					(uint64_t)entry < (uint64_t)xsdt + xsdt->Length; entry++) {
				gen = (const volatile struct acpi_gen_header_t*)*entry;
				map_header(gen);

				//FADT
				if (!memcmp((uint8_t*)gen->Signature, "FACP", 4)) {
					map_table(gen);
					if (verify_checksum(gen)) {
						logging_log_warning("Bad ACPI FACP checksum. Falling back to RSDPV1");
						goto fallback;
					}

					const volatile struct acpi_fadt_t* fadt = (const volatile struct acpi_fadt_t*)gen;
					if (memcmp((uint8_t*)xsdt->OEMTableID, (uint8_t*)fadt->OEMTableID, 8)) {
						logging_log_warning("Bad ACPI FACP OEM Table ID. Falling back to RSDPV1");
						goto fallback;
					}

					logging_log_info("Found ACPI FACP @ 0x%X64", (uint64_t)fadt);

					found |= FOUND_FADT;
				}

				//MADT
				if (!memcmp((uint8_t*)gen->Signature, "APIC", 4)) {
					map_table(gen);
					if (verify_checksum(gen)) {
						logging_log_warning("Bad ACPI MADT checksum. Falling back to RSDPV1");
						goto fallback;
					}

					const volatile struct acpi_madt_t* madt = (const volatile struct acpi_madt_t*)gen;

					logging_log_info("Found ACPI MADT @ 0x%X64", (uint64_t)madt);

					index_io_apics(madt);
					found |= FOUND_MADT;
				}

				//HPET
				if (!memcmp((uint8_t*)gen->Signature, "HPET", 4)) {
					map_table(gen);
					if (verify_checksum(gen)) {
						logging_log_warning("Bad ACPI HPET checksum. Falling back to RSDPV1");
						goto fallback;
					}

					const volatile struct acpi_hpet_t* hpet = (const volatile struct acpi_hpet_t*)gen;

					logging_log_info("Found ACPI HPET @ 0x%X64", (uint64_t)hpet);

					acpi_index.hpet.base = (uint32_t)hpet->BASE_ADDRESS.Address; // only lower 32b are valid
					acpi_index.hpet.min_clock_tick = hpet->MainCounterMinimumClockTick;
					found |= FOUND_HPET;
				}
			}

			if (found != (FOUND_FADT | FOUND_MADT | FOUND_HPET)) {
				logging_log_warning("Could not find all ACPI tables. Falling back to RSDPV1");
				goto fallback;
			}
			break;
	}
}

struct acpi_io_apic_list_t* acpi_get_io_apics(void) {
	return acpi_index.io_apics;
}

struct acpi_hpet_base_t acpi_get_hpet_base(void) {
	return acpi_index.hpet;
}
