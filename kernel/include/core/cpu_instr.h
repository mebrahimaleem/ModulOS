/* cpu_instr.h - cpu instruction interface */
/* Copyright (C) 2025-2026  Ebrahim Aleem
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

#ifndef KERNEL_CORE_CPU_INSTR_H
#define KERNEL_CORE_CPU_INSTR_H

#include <stdint.h>

#include <kernel/core/proc_data.h>

extern void cpu_lidt(uint64_t idt_ptr);

extern void cpu_ltr_28(void);

extern void cpu_cli(void);

extern void cpu_sti(void);

extern void cpu_pause(void);

extern void cpu_invlpg(uint64_t addr);

extern void cpu_wait_loop(void) __attribute__((noreturn));

extern void cpu_halt_loop(void) __attribute__((noreturn));

extern void cpu_trap(void);

extern uint64_t cpu_read_cr2(void);

extern void cpu_wbinvd(void);

extern uint64_t cpu_get_cr3(void);

extern void cpu_set_cr3(uint64_t cr3);

extern void cpu_hlt(void);

static inline void cpu_sti_if(void) {
	if (proc_data_get()->sts & PROC_STS_INT_READY) {
		cpu_sti();
	}
}

static inline void cpu_cli_if(void) {
	cpu_cli();
}

extern void cpu_set_cr4(uint64_t bits);

extern uint64_t cpu_get_fsbase(void);

extern void cpu_set_fsbase(uint64_t base);

extern void cpu_save_fx(uint8_t* data);

extern void cpu_restore_fx(uint8_t* data);

extern void cpu_init_fx(void);

#endif /* KERNEL_CORE_CPU_INSTR_H */
