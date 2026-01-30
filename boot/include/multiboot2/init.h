/* init.h - multiboot2 kernel init interface */
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

#ifndef BOOT_MULTIBOOT2_INIT_H
#define BOOT_MULTIBOOT2_INIT_H

#define MBITAG_TYPE_MEMMAP	6
#ifdef GRAPHICSBASE
#define MBITAG_TYPE_FRMBUF	8
#endif /* GRAPHICSBASE */
#define MBITAG_TYPE_RSDPV2	15

#ifndef _ASM

struct mb2_info_t;

extern void multiboot2_init(struct mb2_info_t* info);

#endif /* _ASM */

#endif /* BOOT_MULTIBOOT2_INIT_H */
