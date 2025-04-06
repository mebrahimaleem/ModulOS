/* kernel.ld - kernel linker script */
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

KERNEL_VMA = -0x80000000;

SECTIONS {
	. = 0x7000;

	.text.real BLOCK(4K) : {
		*(.text.real)
	}

	. = 0x1000;
	.text.mpboot BLOCK(4K) : {
		*(.text.mpboot)
	}

	ASSERT(. <= KERNEL_VMA + 0x80000, "Error: Core kernel has overlap with EBDA")

	. = 1M;

	.text.boot BLOCK(4K) : {
		KEEP(*(.text.multiboot))
		*(.text.boot)
	}

	.data.boot BLOCK(4K) : {
		*(.data.boot)
	}

	.bss.boot BLOCK(4K) : {
		*(.bss.boot)
	}
	
	. = KERNEL_VMA + ADDR(.bss.boot) + SIZEOF(.bss.boot) + 4K - (SIZEOF(.bss.boot) % 4K);
	
	.text BLOCK(4K) : AT (ADDR(.bss.boot) + SIZEOF(.bss.boot) + 4K - (SIZEOF(.bss.boot) % 4K)) {
		*(.text)
	}

	.data BLOCK(4K) : AT (ADDR(.bss.boot) + SIZEOF(.bss.boot) + SIZEOF(.text) + 4K - (SIZEOF(.bss.boot) % 4K) + 4K - (SIZEOF(.text) % 4K)) {
		*(.data)
		*(.rodata*)
	}

	.bss BLOCK(4K) : AT (ADDR(.bss.boot) + SIZEOF(.bss.boot) + SIZEOF(.text) + SIZEOF(.data) + 4K - (SIZEOF(.bss.boot) % 4K) + 4K - (SIZEOF(.text) % 4K) + 4K - (SIZEOF(.data) % 4K)) {
		*(.bss)
	}

	ASSERT(. <= KERNEL_VMA + 4M, "Error: Core kernel exceeds 4MiB limit")

	_kheap_shared = . + 4K - (. % 4K);
	_kheap_private = (-0x1 - kheap_shared + 0x1) / 0x2 + kheap_shared + 4K - (((-0x1 - kheap_shared + 0x1) / 0x2 + kheap_shared) % 4K) - 0x00000000;
}
