
.section .text

.globl _start
_start:
movq $0xdeadbeef, %rax
pushq %rax
hang:
jmp hang
