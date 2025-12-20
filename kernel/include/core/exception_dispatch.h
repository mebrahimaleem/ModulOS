/* exception_dispatch.h - exceptions dispatch interface */
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

#ifndef CORE_EXCEPTION_DISPATCH_H
#define CORE_EXCEPTION_DISPATCH_H

#include <stdint.h>

struct exception_context_t {
	uint64_t rsp;
	uint64_t rbp;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;
	uint64_t vector;
	uint64_t code;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t inter_rsp;
	uint64_t inter_ss;
} __attribute__((packed));

extern void exception_dispatch(struct exception_context_t* context) __attribute__((noreturn));

#endif /* CORE_EXCEPTION_DISPATCH_H */
