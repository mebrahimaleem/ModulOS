/* mp.ld - MP boostrap linker script */
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

PHDRS {
	mptext PT_LOAD FLAGS(1 | 4); /* XR */
	mpdata PT_LOAD FLAGS(2 | 4); /* WR */
}

SECTIONS {
	. = 0x1000;

	.text.mpboot BLOCK(4K) : {
		*(.text.mpboot)
	} :mptext

	.data.mpboot BLOCK(4K) : {
		*(.text.mpboot)
	} :mpdata

	ASSERT(. <= 0x9fc00, "Kernel has overlap with EBDA")
}
