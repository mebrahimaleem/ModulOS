
.section .text

.globl _start
_start:
movq $1, %rdi

syscall

movq $0, %rdi
movq $1, %rsi
pushq $42
movq %rsp, %rdx

syscall
