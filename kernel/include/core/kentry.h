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

#ifndef CORE_KENTRY_H
#define CORE_KENTRY_H

#include <stdint.h>
#include <stddef.h>

#include <core/acpitables.h>

#ifdef GRAPHICSBASE
#include <graphicsbase/framebuffer.h>
#endif /* GRAPHICSBASE */

struct boot_context_t {
	size_t num_memmap;
	struct memmap_t* memmap;
	struct RSDP_t rsdp;
#ifdef GRAPHICSBASE
	struct framebuffer_t framebuffer;
#endif /* GRAPHICSBASE */
};

extern void kentry(void) __attribute__((noreturn));

#endif /* CORE_KENTRY_H */
