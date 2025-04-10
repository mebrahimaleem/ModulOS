/* panicasm.S - panic assembly routines */
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

#ifndef CORE_PANICASM_S
#define CORE_PANICASM_S

	.code64
	.text

/* __noreturn__ void panic_hlt(void); */
	.globl panic_hlt
panic_hlt:
	hlt
	jmp panic_hlt

#endif /* CORE_PANICASM_S */
