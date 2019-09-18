##
## @file boot.S
##
## @author Angelo Elias Dalzotto (150633@upf.br)
##
## @brief Initializes the stack pointer and jumps to kernel master main
##

.section .init
.globl _start
_start:
	li		sp,sp_addr
	jal		main
$L1:
	j $L1
