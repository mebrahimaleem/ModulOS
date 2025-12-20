/* cpu_instr.h - cpu instruction interface */
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

#ifndef CORE_CPU_INSTR_H
#define CORE_CPU_INSTR_H

#include <stdint.h>

extern void cpu_lidt(uint64_t idt_ptr);

extern void cpu_ltr_28(void);

extern void cpu_cli(void);

extern void cpu_sti(void);

extern void cpu_halt_loop(void) __attribute__((noreturn));

#endif /* CORE_CPU_INSTR_H */
