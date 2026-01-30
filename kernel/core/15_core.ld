/* 15_kernel.ld - kernel linker script */
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

KERNEL_VMA = -0x80000000;
MAXMEM = -0x1;

PHDRS {
	kerneltext PT_LOAD FLAGS(1 | 4); /* XR */
	kerneldata PT_LOAD FLAGS(2 | 4); /* WR */
}

SECTIONS {
	. = KERNEL_VMA + BOOT_END;
	. = ALIGN(4K);

	KERNEL_START = .;

	.text ALIGN(4K) : AT (ADDR(.text) - KERNEL_VMA) {
		*(.text*)
	} :kerneltext

	. = ALIGN(4K);

	.data ALIGN(4K) : AT (ADDR(.data) - KERNEL_VMA) {
		*(.data*)
		*(.rodata*)
	} :kerneldata

	. = ALIGN(4K);

	.bss ALIGN(4K) : AT (ADDR(.bss) - KERNEL_VMA) {
		*(.bss*)
	} :kerneldata

	ASSERT(. <= KERNEL_VMA + 4M, "Error: Kernel exceeds 4MiB limit")

	. = ALIGN(2M);
	_kernel_pend = . - KERNEL_VMA;
	_kernel_heap_base = .;

	. = MAXMEM;
	_kernel_limit = .;
}
