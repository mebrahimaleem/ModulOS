/* kentry.h - kernel entry point interface */
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

#ifndef KERNEL_CORE_KENTRY_H
#define KERNEL_CORE_KENTRY_H

#include <stdint.h>
#include <stddef.h>

#include <kernel/core/acpitables.h>
#include <kernel/core/gdt.h>

#ifdef GRAPHICSBASE
#include <kernel/graphicsbase/framebuffer.h>
#endif /* GRAPHICSBASE */

struct boot_context_t {
	struct RSDP_t rsdp;
	volatile struct gdt_t(* gdt)[GDT_NUM_ENTRIES];
#ifdef GRAPHICSBASE
	struct framebuffer_t framebuffer;
#endif /* GRAPHICSBASE */
};

extern struct boot_context_t boot_context;

extern void kentry(void) __attribute__((noreturn));

#endif /* KERNEL_CORE_KENTRY_H */
