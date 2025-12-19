/* kentry.c - kernel entry point */
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

#include <core/kentry.h>
#include <core/tss.h>
#include <core/idt.h>
#include <core/cpu_instr.h>

#include <drivers/pic_8259/pic.h>

struct boot_context_t boot_context;

void kentry(void) {
	tss_init();
	idt_init();

	pic_disab();

	cpu_sti();

	while (1);
}
