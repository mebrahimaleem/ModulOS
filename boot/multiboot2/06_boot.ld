/* boot.ld - boot linker script */
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

ENTRY(_start)

PHDRS {
	boottext PT_LOAD FLAGS(1 | 4); /* XR */
	bootdata PT_LOAD FLAGS(2 | 4); /* WR */
}

SECTIONS {
	. = 1M;

	.text.boot BLOCK(4K) : {
		KEEP(*(.text.multiboot2))
		*(.text.boot)
	} :boottext

	.data.boot BLOCK(4K) : {
		*(.data.boot)
	} :bootdata

	.bss.boot BLOCK(4K) : {
		*(.bss.boot)
	} :bootdata

	BOOT_END = .;
}
