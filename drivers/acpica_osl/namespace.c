/* namespace.c - acpica subsystem namespace init */
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

#include <acpica_osl/namespace.h>
#include <acpica_osl/acpica_include.h>

#include <kernel/core/logging.h>
#include <kernel/core/panic.h>
#include <kernel/core/alloc.h>

#define INDEX_PRT 0x1

#define NAME_TO_INT(a, b, c, d) (((uint64_t)a) | ((uint64_t)b << 8) | ((uint64_t)c << 16) | ((uint64_t)d << 24))

#define NAME__PRT NAME_TO_INT('_', 'P', 'R', 'T')
#define NAME__CRS NAME_TO_INT('_', 'C', 'R', 'S')

#define CRS_TYPE_SIZE_MASK	0x80
#define CRS_TYPE_SIZE_SMALL	0x00
#define CRS_TYPE_SIZE_LARGE	0x80

#define CRS_SMALL_TYPE_MASK	0x78
#define CRS_LARGE_TYPE_MASK 0x7F

#define CRS_SMALL_LEN_MASK	0x7

#define CRS_TAG_END					0x78

static uint64_t index_res;

struct pci_routing_t {
	uint8_t pci_pin;
	uint64_t gsi;
	struct pci_routing_t* n;
};

union crs_elem_t {
	uint8_t type;
	struct {
		uint8_t type;
		uint8_t dat[];
	} __attribute__((packed)) small_res;

	struct {
		uint8_t type;
		uint8_t len_lo;
		uint8_t len_hi;
		uint8_t dat[];
	} __attribute__((packed)) large_res;
};

static struct pci_routing_t* pci_routing;

static ACPI_STATUS callback_skip(ACPI_HANDLE obj, UINT32 lvl, void* cntx, void** ret) {
	(void)obj;
	(void)lvl;
	(void)cntx;
	(void)ret;

	return AE_OK;
}

static ACPI_STATUS callback_walk_device(ACPI_HANDLE obj, UINT32 lvl, void* cntx, void** ret) {
	(void)lvl;
	(void)cntx;
	(void)ret;

	ACPI_STATUS sts;
	ACPI_BUFFER buf = {.Pointer = NULL};

	ACPI_DEVICE_INFO* info;
	sts = AcpiGetObjectInfo(obj, &info);

	if (ACPI_FAILURE(sts)) {
		logging_log_error("Failed to get object info : 0x%x64", (uint64_t)sts);
		return sts;
	}

	switch (info->Name) {
		case NAME__CRS:
			buf.Length = ACPI_ALLOCATE_BUFFER;
			buf.Pointer = NULL;

			sts = AcpiEvaluateObject(obj, NULL, NULL, &buf);
			if (ACPI_FAILURE(sts)) {
				logging_log_error("Failed to evaluate ACPI _CRS object: 0x%x64", (uint64_t)sts);
				goto cleanup;
			}

			if (((ACPI_OBJECT*)buf.Pointer)->Type != ACPI_TYPE_BUFFER) {
				logging_log_error("ACPI _CRS has wrong type: 0x%x64", ((ACPI_OBJECT*)buf.Pointer)->Type);
				sts = AE_TYPE;
				goto cleanup;
			}

			union crs_elem_t* elem = (union crs_elem_t*)((ACPI_OBJECT*)buf.Pointer)->Buffer.Pointer;
			while (1) {
				switch (elem->type & CRS_TYPE_SIZE_MASK) {
					case (CRS_TYPE_SIZE_SMALL):
						logging_log_info("Found small CRS entry type: 0x%x64", elem->type & CRS_SMALL_TYPE_MASK);
						switch (elem->type & CRS_SMALL_TYPE_MASK) {
							case CRS_TAG_END:
								sts = AE_OK;
								goto cleanup;
							default:
								elem = (union crs_elem_t*)((uint64_t)elem + 1 + (elem->type & CRS_SMALL_LEN_MASK));
								break;
						}
						break;
					case (CRS_TYPE_SIZE_LARGE):
						logging_log_info("Found large CRS entry type: 0x%x64", elem->type & CRS_LARGE_TYPE_MASK);
						elem = (union crs_elem_t*)(
								(uint64_t)elem + 3 + (uint64_t)(elem->large_res.len_lo) + ((uint64_t)elem->large_res.len_hi << 8));
						break;
				}
			}

		cleanup:
			if (buf.Pointer) {
				AcpiOsFree(buf.Pointer);
			}

			logging_log_info("Indexed ACPI namespace %4.4s", &info->Name);
			return sts;
		default:
			return AcpiWalkNamespace(ACPI_TYPE_ANY, obj, 1, callback_walk_device, callback_skip, NULL, NULL);
	}
}

static ACPI_STATUS callback_walk_root(ACPI_HANDLE obj, UINT32 lvl, void* cntx, void** ret) {
	(void)lvl;
	(void)cntx;
	(void)ret;

	ACPI_STATUS sts;
	ACPI_DEVICE_INFO* info;
	sts = AcpiGetObjectInfo(obj, &info);

	if (ACPI_FAILURE(sts)) {
		logging_log_error("Failed to get object info : 0x%x64", (uint64_t)sts);
		return sts;
	}

	uint64_t i;

	struct pci_routing_t* tmp_routing;

	ACPI_BUFFER buf = {.Pointer = NULL};

	switch (info->Name) {
		case NAME__PRT:
			/* _PRT */
			buf.Length = ACPI_ALLOCATE_BUFFER;
			buf.Pointer = NULL;

			sts = AcpiEvaluateObject(obj, NULL, NULL, &buf);
			if (ACPI_FAILURE(sts)) {
				logging_log_error("Failed to evaluate ACPI _PRT object: 0x%x64", (uint64_t)sts);
				goto cleanup;
			}

			ACPI_OBJECT* prt = buf.Pointer;
			if (prt->Type != ACPI_TYPE_PACKAGE) {
				logging_log_error("_PRT has wrong type: 0x%x64", (uint64_t)prt->Type);
				sts = AE_TYPE;
				goto cleanup;
			}

			for (i = 0; i < prt->Package.Count; i++) {
				volatile ACPI_OBJECT* entry = &(prt->Package.Elements[i]);

				if (entry->Type != ACPI_TYPE_PACKAGE) {
					logging_log_error("_PRT package entry has wrong type: 0x%x64", (uint64_t)entry->Type);
					sts = AE_TYPE;
					goto cleanup;
				}

				if (entry->Package.Count != 4) {
					logging_log_error("_PRT package entry has wrong count: 0x%d64", (uint64_t)entry->Package.Count);
					sts = AE_TYPE;
					goto cleanup;
				}

				volatile ACPI_OBJECT* field = &entry->Package.Elements[1];
				
				if (field->Type != ACPI_TYPE_INTEGER) {
					logging_log_error("_PRT package entry has bad pin value");
					sts = AE_TYPE;
					goto cleanup;
				}

				const uint8_t pin = (uint8_t)field->Integer.Value;

				// find source
				field = &entry->Package.Elements[2];
				if (field->Type == ACPI_TYPE_LOCAL_REFERENCE 
						&& field->Reference.ActualType == ACPI_TYPE_DEVICE) {
					AcpiWalkNamespace(
							ACPI_TYPE_ANY,
							field->Reference.Handle,
							1,
							callback_walk_device,
							callback_skip,
						 	NULL,
							NULL);
				}
				else if (field->Type == ACPI_TYPE_INTEGER && field->Integer.Value == 0) {
					field = &entry->Package.Elements[3];
					
					if (field->Type != ACPI_TYPE_INTEGER) {
						logging_log_error("_PRT package entry has bad source index value");
						sts = AE_TYPE;
						goto cleanup;
					}

					tmp_routing = pci_routing;
					pci_routing = kmalloc(sizeof(struct pci_routing_t));
					pci_routing->pci_pin = pin;
					pci_routing->gsi = field->Integer.Value;
					pci_routing->n = tmp_routing;
				}
				else {
					logging_log_error("_PRT package entry has bad source type %d64", field->Type);
					sts = AE_TYPE;
					goto cleanup;
				}
			}
		
			sts = AE_OK;
			index_res |= INDEX_PRT;
			goto cleanup;

cleanup:
			if (buf.Pointer) {
				AcpiOsFree(buf.Pointer);
			}

			logging_log_info("Indexed ACPI namespace %4.4s", &info->Name);
			return sts;
		default:
			return AcpiWalkNamespace(ACPI_TYPE_ANY, obj, 1, callback_walk_root, callback_skip, NULL, NULL);
	}
}

void acpica_index_all(void) {
	index_res = 0;
	pci_routing = 0;

	// manually subwalk for finer control
	ACPI_STATUS sts = AcpiWalkNamespace(
			ACPI_TYPE_ANY,
			ACPI_ROOT_OBJECT,
			1,
			callback_walk_root,
			callback_skip,
			NULL,
			NULL);

	if (ACPI_FAILURE(sts)) {
		logging_log_error("Failed to ACPICA walk the namespace: 0x%x64", (uint64_t)sts);
		panic(PANIC_ACPI);
	}

	if (index_res ^ (INDEX_PRT)) {
		logging_log_error("Failed to index all required namespaces. index_res: 0x%x64", index_res);
	}
}

