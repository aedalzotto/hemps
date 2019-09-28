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

	#https://stackoverflow.com/questions/50214840/risc-v-startup-code-for-stack-pointer-initalization?rq=1
	# Setup the global pointer, which the ABI assumes points to the
    # __global_pointer$ symbol. 
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

	# Set up the stack pointer
	li		sp,sp_addr

	# Call main
	jal		main

	# Spin after main() returns
$L1:
	j $L1
