/* isr_dispatch.c - APIC ISR dispatcher */
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

#include <apic/isr_dispatch.h>
#include <apic/apic_regs.h>

#include <kernel/core/logging.h>

#define ESR_IRA	0x80
#define ESR_RIV	0x40
#define ESR_SIV	0x20
#define ESR_RAE	0x08
#define ESR_SAE	0x04

void apic_error_dispatch(void) {
	const char* ira = "";
	const char* riv = "";
	const char* siv = "";
	const char* rae = "";
	const char* sae = "";
	const uint32_t esr = apic_read_reg(APIC_REG_ESR);

	if ((esr & ESR_IRA) == ESR_IRA) {
		ira = " Illegal Register Access";
	}

	if ((esr & ESR_RIV) == ESR_RIV) {
		riv = " Recieved Illegal Vector";
	}

	if ((esr & ESR_SIV) == ESR_SIV) {
		siv = " Sent Illegal Vector";
	}

	if ((esr & ESR_RAE) == ESR_RAE) {
		rae = " Recieve Accept Error";
	}

	if ((esr & ESR_SAE) == ESR_SAE) {
		sae = " Sent Accept Error";
	}

	logging_log_error("APIC error:%s%s%s%s%s", ira, riv, siv, rae, sae);
	apic_write_reg(APIC_REG_ESR, 0);
	apic_write_reg(APIC_REG_EOI, APIC_EOI);
}
