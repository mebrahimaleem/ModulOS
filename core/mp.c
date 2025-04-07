/* mp.c - multiple processor management */
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

#ifndef CORE_MP_C
#define CORE_MP_C

#define CMOS_ADDR						0x70
#define CMOS_DATA						0x71

#define REG_STS_A						0x0A
#define REG_STS_B						0x0B
#define REG_STS_C						0x0C
#define REG_SHUTDOWN				0x0F

#define SHUTDOWN_AP_STARTUP	0x0A
#define PERIODIC_INT				0x40
#define RATE_MASK						0xF0
#define DESIRED_RATE				0x06

#define PERIODIC_FLAG				0x40

#define NMI_DISABLE					0x80

#define WARM_RESET_VOFF			0x467
#define WARM_RESET_VSEG			0x469

#define LOADING_WAITSIPI		0x0
#define LOADING_BOOTSTRAP		0x1
#define LOADING_IDLE				0x2

#include <core/atomic.h>
#include <core/portio.h>
#include <core/memory.h>
#include <core/mp.h>
#include <core/kentry.h>

#include <acpi/madt.h>

#include <apic/lapic.h>

void mp_initall() {
	kcli();

	/* set bios shutdown to 0x0A */
	outb(CMOS_ADDR, REG_SHUTDOWN | NMI_DISABLE);
	outb(CMOS_DATA, SHUTDOWN_AP_STARTUP);

	/* set warm reset vector */
	*(uint16_t* volatile)(WARM_RESET_VSEG) = (uint16_t)((uint64_t)&mp_bootstrap >> 4);
	*(uint16_t* volatile)(WARM_RESET_VOFF) = (uint16_t)0; // since ap startup routine is 4K aligned

	uint8_t reg;
	/* temporarly program RTC timer for polling 1024Hz */
	outb(CMOS_ADDR, REG_STS_B | NMI_DISABLE);
	reg = inb(CMOS_DATA);
	outb(CMOS_ADDR, REG_STS_B | NMI_DISABLE);
	outb(CMOS_DATA, reg | PERIODIC_INT);

	outb(CMOS_ADDR, REG_STS_A | NMI_DISABLE);
	reg = inb(CMOS_DATA) & RATE_MASK;
	outb(CMOS_ADDR, REG_STS_A | NMI_DISABLE);
	outb(CMOS_DATA, reg | DESIRED_RATE);

	const uint8_t bspId = apic_getId();

	//TODO: store ids somewhere
	uint64_t hint = 0;
	uint8_t i;
	struct acpi_lapic* lapic;

	while (1) {
		hint = acpi_nextAPIC(hint, &lapic);

		/* check if end of MADT */
		if (hint == 0) {
			break;
		}
		
		/* skip if BSP */
		if (lapic->apicId == bspId) {
			continue;
		}

		/* setup trampoline */
		uint64_t* gdt = kcalloc(kheap_shared, sizeof(uint64_t) * 16, 1);
		gdt[1] = 0x00a09a000000ffff;
		gdt[2] = 0x00a092000000ffff;
		
		mp_loading = LOADING_WAITSIPI;
		mp_rsp = (uint64_t)kmalloc(kheap_shared, 0x4000) + 0x4000;
		mp_gdtptr.off = calculatePaddr(kPML4T, (uint64_t)gdt);

		/* send init sipi sipi */
		for (i = 0; i < 20; ) {
			/* poll until PF is set */
			outb(CMOS_ADDR, REG_STS_C | NMI_DISABLE);
			reg = inb(CMOS_DATA);
			if ((reg & PERIODIC_FLAG) != PERIODIC_FLAG) {
				continue;
			}

			switch (i) {
				case 0:
					/* first time: send init */
					// TODO: send targetted
					apic_lapic_sendipi(0, LAPIC_IPI_INIT | LAPIC_IPI_PHYSC | LAPIC_IPI_ASSERT | LAPIC_IPI_LEVL | LAPIC_IPI_NOSHORT, lapic->apicId);
					apic_lapic_waitForIpi();
					apic_lapic_sendipi(0, LAPIC_IPI_INIT | LAPIC_IPI_PHYSC | LAPIC_IPI_DEASSERT | LAPIC_IPI_LEVL | LAPIC_IPI_NOSHORT, lapic->apicId);
					apic_lapic_waitForIpi();
					break;
				case 10:
					/* wait ~10ms, then send sipi */
					apic_lapic_sendipi((uint8_t)((uint64_t)&mp_bootstrap >> 12), LAPIC_IPI_STARTUP | LAPIC_IPI_PHYSC | LAPIC_IPI_ASSERT | LAPIC_IPI_EDGE | LAPIC_IPI_NOSHORT, lapic->apicId);
					apic_lapic_waitForIpi();
					break;
				case 12:
					/* wait ~2ms, then send another sipi */
					if (mp_loading == LOADING_WAITSIPI) {
						apic_lapic_sendipi((uint8_t)((uint64_t)&mp_bootstrap >> 12), LAPIC_IPI_STARTUP | LAPIC_IPI_PHYSC | LAPIC_IPI_ASSERT | LAPIC_IPI_EDGE | LAPIC_IPI_NOSHORT, lapic->apicId);
						apic_lapic_waitForIpi();
					}
					break;
				default:
					break;
			}
			i++;
		}
		/* check for failure */
		if (mp_loading == LOADING_WAITSIPI) {
			//TODO: handle error, maybe retry
			continue;
		}

		/* wait for AP to finish bootstrapping */
		while (mp_loading == LOADING_BOOTSTRAP);
	}

	ksti();
}

#endif /* CORE_MP_C */

