/* exception_dispatch.h - exceptions dispatcher */
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

#include <stdint.h>

#include <core/exception_dispatch.h>
#include <core/logging.h>
#include <core/panic.h>

void exception_dispatch(struct exception_context_t* context) {
	//TODO: handle from userland	

	logging_log_error("Unrecoverable exception 0x%X64 (0x%X64) @ 0x%X64",
			context->vector, context->code, context->rip);

	logging_log_debug("Register Dump:\r\nrax 0x%X64\r\nrbx 0x%X64\r\nrcx 0x%X64\r\nrdx 0x%X64\
\r\nrsi 0x%X64\r\nrdi 0x%X64\r\nrbp 0x%X64\r\nrsp 0x%X64\r\nr8  0x%X64\r\nr9  0x%X64\r\nr10 0x%X64\
\r\nr11 0x%X64\r\nr12 0x%X64\r\nr13 0x%X64\r\nr14 0x%X64\r\nr15 0x%X64\r\nrfl 0x%X64\r\ncs  0x%X64\r\nrip 0x%X64",
			context->rax, context->rbx, context->rcx, context->rdx,
			context->rsi, context->rdi, context->rbp, context->rsp,
			context->r8, context->r9, context->r10, context->r11,
			context->r12, context->r13, context->r14, context->r15,
			context->rflags, context->cs, context->rip);

	panic(PANIC_STATE);
}
