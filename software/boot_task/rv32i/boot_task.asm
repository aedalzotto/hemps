## @author Angelo Elias Dalzotto (150633@upf.br)
##
## @brief Initializes the stack pointer and jumps to main(). Handles the SystemCall() from C to syscall.

.section .text		# Start of ".text" section (means code section)
.align  2			# Aligns to 2^2 = 4 bytes

.globl  entry   	# Exports entry symbol
entry:
	li sp, sp_addr	# Stack pointer address passed on makefile. Loads the page size to the sp.

	# JAL: Jump and Link
   	# Copies the address of the next instruction into the register ra
	jal main		# Execute main
   
	mv a0, zero		# On return from main execute syscall with argument "0"
	ecall			# Syscall address = ??
   
$L1:             	# Executes always that returns from syscall. But does the syscall returns?
	j $L1          	# infinite loop
  
#####################

.globl SystemCall	# "registers that a global SystemCall function exists to C code"
SystemCall:
	ecall			# Syscall address = ??
   
	# JR: Jump to register
	# Most commonly used jr ra
	ret 			# Returns from syscall. $ra has the return address of the callee.
