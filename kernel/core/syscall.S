/* process.S - kernel system call */
/* Copyright (C) 2026  Ebrahim Aleem
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

#include <core/syscall_vectors.h>

.section .text

.globl syscall_return
syscall_return:
movq %rdi, %rcx // rip
movq %rsi, %r11 // rflags
movq %rdx, %rsp // rsp
movq %rcx, %rax // return value

sysretq

.globl syscall_entry
syscall_entry:
cmpq $SYSCALL_MAX, %rax
jae syscall_fail

pushq %rbp
movq %rsp, %rbp

movq %gs:0, %rsp
movq (%rsp), %rsp

pushq %rcx
pushq %r11

movq %r10, %rcx

call *syscall_handlers(,%rax,8)

popq %r11
popq %rcx

movq %rbp, %rsp
popq %rbp

sysretq

syscall_fail:
movq $SYSCALL_STS_FAIL, %rax
sysretq

.section .rodata

.global syscall_handlers
syscall_handlers:

.quad syscall_dispatch_exit
.quad syscall_dispatch_openat
.quad syscall_dispatch_close
.quad syscall_dispatch_read
.quad syscall_dispatch_write
.quad syscall_dispatch_alloc
.quad 0
.quad 0
.quad syscall_dispatch_open_dir
.quad syscall_dispatch_read_dir
.quad syscall_dispatch_truncate
.quad syscall_dispatch_seek
.quad syscall_dispatch_tell
.quad syscall_dispatch_create_dir
.quad syscall_dispatch_delete_dir
.quad syscall_dispatch_epoch_time
.quad syscall_dispatch_is_a_tty
.quad syscall_dispatch_gcwd
.quad syscall_dispatch_ccwd
.quad syscall_dispatch_link
.quad syscall_dispatch_unlink
.quad syscall_dispatch_stat

.set num_entries, . - syscall_handlers
.if num_entries != SYSCALL_MAX * 8
.error "Inconsistent number of registered system cals in call table"
.endif
