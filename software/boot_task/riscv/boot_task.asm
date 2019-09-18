## @author Angelo Elias Dalzotto (150633@upf.br)
##
## @brief Initializes the stack pointer and jumps to main(). Handles the SystemCall() from C to syscall.

.section .init		# Start of ".init" section (means bootloader)

.globl _start  	# Exports entry symbol
_start:
	li sp, sp_addr	# Stack pointer address passed on makefile. Loads the page size to the sp.

	# JAL: Jump and Link
   	# Copies the address of the next instruction into the register ra
	jal main		# Execute main
   
	mv a0, zero		# On return from main execute syscall with argument "0"
	ecall			# Syscall address = set by kernel
$L1:             	# Executes always that returns from syscall.
	j $L1          	# infinite loop
  
#####################

.section .text

.globl SystemCall	# "registers that a global SystemCall function exists to C code"
SystemCall:
	ecall			# Syscall address = set by kernel
   
	ret 			# Returns from syscall. $ra has the return address of the callee.
