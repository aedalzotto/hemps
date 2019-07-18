##
## @file boot.S
##
## @author Angelo Elias Dalzotto (150633@upf.br)
##
## @brief Initializes the stack pointer and jumps to kernel master main
##

.section .text
.align  2
.globl boot_master
boot_master:
	li		sp,sp_addr
	jal		main
$L1:
	j $L1
