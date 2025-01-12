/* init.S - kernel init code */
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

#ifndef MULTIBOOT_INIT_S
#define MULTIBOOT_INIT_S
	
	.code32
	.text

	.globl pm2long
pm2long:
	push %ebx
	push %eax /* later passed to kernel */

	mov $msg_loading, %esi
	call pm_print
	
	/* TODO: enter long mode */

	/* TODO: call kernel */

	/* fallback halt */
loop:
	hlt
	jmp		loop

pm_print: // pm_print, esi points to string, modifies esi
	push %eax
	push %ebx

	mov (vga_offset), %eax

pm_print.loop:
		movb (%esi), %bl
		test %bl, %bl
		jz pm_print.done
		movb %bl, (%eax)
		inc %eax

		movb $0x0F, (%eax)
		inc %eax
		inc %esi
		jmp pm_print.loop

pm_print.done:
	
	mov %eax, (vga_offset)

	pop %ebx
	pop %eax
	ret

	.data

msg_loading:
	.asciz "Loading..."

vga_offset:
	.long 0xb8000

#endif /* MULTIBOOT_INIT_S */
