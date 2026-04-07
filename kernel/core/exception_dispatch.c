/* exception_dispatch.h - exceptions dispatcher */
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

#include <stdint.h>

#include <core/exception_dispatch.h>
#include <core/logging.h>
#include <core/panic.h>
#include <core/paging.h>
#include <core/cpu_instr.h>

#define VECTOR_DE		0x00
#define VECTOR_DB		0x01
#define VECTOR_NMI	0x02
#define VECTOR_BP		0x03
#define VECTOR_OF		0x04
#define VECTOR_BR		0x05
#define VECTOR_UD		0x06
#define VECTOR_NM		0x07
#define VECTOR_DF		0x08
#define VECTOR_TS		0x0A
#define VECTOR_NP		0x0B
#define VECTOR_SS		0x0C
#define VECTOR_GP		0x0D
#define VECTOR_PF		0x0E
#define VECTOR_MF		0x10
#define VECTOR_AC		0x11
#define VECTOR_MC		0x12
#define VECTOR_XM		0x13
#define VECTOR_VE		0x14
#define VECTOR_CP		0x15
#define VECTOR_HV		0x1C
#define VECTOR_VC		0x1D
#define VECTOR_SX		0x1E

static inline const char* get_exception_name(uint64_t vector) {
	switch (vector) {
		case VECTOR_DE:
			return "#DE";
		case VECTOR_DB:
			return "#DB";
		case VECTOR_NMI:
			return "NMI";
		case VECTOR_BP:
			return "#BP";
		case VECTOR_OF:
				return "#OF";
		case VECTOR_BR:
				return "#BR";
		case VECTOR_UD:
				return "#UD";
		case VECTOR_NM:
				return "#NM";
		case VECTOR_DF:
				return "#DF";
		case VECTOR_TS:
				return "#TS";
		case VECTOR_NP:
				return "#NP";
		case VECTOR_SS:
				return "#SS";
		case VECTOR_GP:
				return "#GP";
		case VECTOR_PF:
				return "#PF";
		case VECTOR_MF:
				return "#MF";
		case VECTOR_AC:
				return "#AC";
		case VECTOR_MC:
				return "#MC";
		case VECTOR_XM:
				return "#XM";
		case VECTOR_VE:
				return "#VE";
		case VECTOR_CP:
				return "#CP";
		case VECTOR_HV:
				return "#HV";
		case VECTOR_VC:
				return "#VC";
		case VECTOR_SX:
				return "#SX";
		default:
				return "";
	}
}

void exception_dispatch(struct exception_context_t* context) {
	//TODO: remove reduntant rsp push
	logging_log_error("Unrecoverable exception 0x%lX %s (0x%lX) @ 0x%lX",
			context->vector, get_exception_name(context->vector), context->code, context->rip);

	logging_log_debug("Register Dump:\r\nrax 0x%lX\r\nrbx 0x%lX\r\nrcx 0x%lX\r\nrdx 0x%lX\
\r\nrsi 0x%lX\r\nrdi 0x%lX\r\nrbp 0x%lX\r\nrsp 0x%lX\r\nr8  0x%lX\r\nr9  0x%lX\r\nr10 0x%lX\
\r\nr11 0x%lX\r\nr12 0x%lX\r\nr13 0x%lX\r\nr14 0x%lX\r\nr15 0x%lX\r\nrfl 0x%lX\r\ncs  0x%lX\
\r\nss  0x%lX\r\nrip 0x%lX",
			context->rax, context->rbx, context->rcx, context->rdx,
			context->rsi, context->rdi, context->rbp, context->rsp,
			context->r8, context->r9, context->r10, context->r11,
			context->r12, context->r13, context->r14, context->r15,
			context->rflags, context->cs, context->ss, context->rip);

	switch (context->vector) {
		case VECTOR_PF:
			if (paging_check_guard(read_cr2())) {
				logging_log_error("Stack Overflow");
			}
			break;
		default:
			break;
	}	

	panic(PANIC_STATE);
}
