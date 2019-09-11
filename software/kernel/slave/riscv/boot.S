##
## @file boot.S
##
## @author Angelo Elias Dalzotto (150633@upf.br)
##
## @brief Initializes the stack pointer and jumps to kernel_slave main
##

.section .text
.align 2

.globl boot_slave
boot_slave:
	li		sp,sp_addr

	# Configure Syscall
	//@todo mtvec.base = exceptions entry
	//@todo mtvec.mode = vectored
	
	# Configure addressing
	// @todo

	jal		main
$L1:
	j		$L1

#########################

exceptions_entry:				# Set to mtvec.BASE and VECTORED
	j		exception_handler	# Exceptions (inc. ecall)
interrupts_entry:				# Interrupts
	j		interrupt_handler	# SSI
	nop
	j		interrupt_handler	# MSI
	nop
	j		interrupt_handler	# STI
	nop
	j		interrupt_handler	# MTI
	nop
	j		interrupt_handler	# SEI
	nop
	j		interrupt_handler	# MEI

interrupt_handler:
	# All interrupts are handled by this function when
	# mtvec.MODE == VECTORED

	# mscratch CSR is reserved to OS
	# Save t0 to mscratch to let kernel use it
	csrw	mscratch,t0
	lw		t0,current		# Load "current" to t0

	# Save registers
	# sw		zeros,0(t0)
	sw		ra,4(t0)
	sw		sp,8(t0)
	sw		gp,12(t0)
	sw		tp,16(t0)
	# sw		t0,20(t0)
	sw		t1,24(t0)
	sw		t2,28(t0)
	sw		s0,32(t0)
	sw		s1,36(t0)
	sw		a0,40(t0)
	sw		a1,44(t0)
	sw		a2,48(t0)
	sw		a3,52(t0)
	sw		a4,56(t0)
	sw		a5,60(t0)
	sw		a6,64(t0)
	sw		a7,68(t0)
	sw		s2,72(t0)
	sw		s3,76(t0)
	sw		s4,80(t0)
	sw		s5,84(t0)
	sw		s6,88(t0)
	sw		s7,92(t0)
	sw		s8,96(t0)
    sw		s9,100(t0)
	sw		s10,104(t0)
	sw		s11,108(t0)
	sw		t3,112(t0)
	sw		t4,116(t0)
	sw		t5,120(t0)
	sw		t6,124(t0)

	csrr	t1,mscratch	# Load t0 to t1 now that t1 has been saved
	sw		t1,20(t0)	# Save t0 that is loaded into t1 to its reg address
	
	li		sp,sp_addr	# Restore stack pointer to top

	// @todo Change kernel_slave, task_control and task_migration to use mepc instead of TCB.pc

	#JUMP TO INTERRUPT SERVICE ROUTINE
	lui		a1,0x2000
	lw		a0,0x20(a1)     #IRQ_STATUS
	lw		a2,0x10(a1)     #IRQ_MASK
	and		a0,a0,a2
	jal		OS_InterruptServiceRoutine

####################################################

exception_handler:
	# ALL synchronous exceptions are heandled by this function, not only ecall.

	# Save t0 to mscratch to let kernel use it
	csrw	mscratch,t0

	csrr	t0,mcause			# Load exception cause to t0
	andi	t0,t0,0xFFFFFFF7	# Clear bit 3
	beqz	t0,ecall_routine	# If cause is ecall, jump to routine

	csrr	t0,mscratch			# Else load back t0 and return to program
	mret						# IMPORTANT: it does not handles exceptions
ecall_routine:
	lw		t0,current		# Load current to t0
        
	# saves minimum context
	sw		a0,40(s10)		# $a0
	sw		a1,44(s10)		# $a1
	sw		a2,48(s10)		# $a2
	sw		a3,52(s10)      # $a3

	sw		a4,52(s10)
	sw		a5,56(s10)
	sw		a6,60(s10)
	sw		a7,64(s10)
	#sw		pc,124(s10)		# TODO: SAVE EPC

	li		sp,sp_addr

	# jumps to system calls handler
	jal		Syscall        
        
	# verifies if scheduling is needed
	la		s10,schedule_after_syscall
	lw		s10,0(s10)
	beqz	s10,system_service_restore

	# saves the context of the running task
	la		a0,current
	lw		a0,0(a0)
	jal		ASM_SaveRemainingContext

	# schedules the next ready task
	jal		Scheduler

	# restores the context and runs the scheduled task
	la		a0,current
	lw		a0,0(a0)
	jal		ASM_RunScheduledTask

system_service_restore:
	la		t6,current
	lw      t6,0(t6)        
        
	# restores the context previously saved
	lw		ra,0(s10)
	lw		sp,4(s10)
	#lw		gp,8(t6)
	
	lw		t5,124(t6)		# loads pc of the task that was running
	#lw		t6,128(t6)		# TODO: loads offset of the task (for paging setup)

	#li		$1,0x1
	#mtc0	t6,$10			# TODO: load offset of task to c0's EntryHi
	#mtc0	$1,$12			# writes 1 to c0's SR, enabling interrupts
	jr		t5				# jumps to pc

###################################################

.globl  ASM_SaveRemainingContext        
ASM_SaveRemainingContext:

	sw		ra,0(a0)		# reg[0] = ra
	sw		sp,4(a0)		# reg[1] = sp
	#sw		gp,8(a0)		# reg[2] = gp
	sw		tp,12(a0)		# reg[3] = tp
	#sw		t0,16(a0)		# reg[4] = t0
	#sw		t1,20(a0)		# reg[5] = t1
	#sw		t2,24(a0)		# reg[6] = t2
	sw		s0,28(a0)		# reg[7] = s0
	sw		s1,32(a0)		# reg[8] = s1
	sw		a0,36(a0)		# reg[9] = a0
	sw		a1,40(a0)		# reg[10] = a1
	sw		a2,44(a0)		# reg[11] = a2
	sw		a3,48(a0)		# reg[12] = a3
	sw		a4,52(a0)		# reg[13] = a4
	sw		a5,56(a0)		# reg[14] = a5
	sw		a6,60(a0)		# reg[15] = a6
	sw		a7,64(a0)		# reg[16] = a7
	sw		s2,68(a0)		# reg[17] = s2
	sw		s3,72(a0)		# reg[18] = s3
	sw		s4,76(a0)		# reg[19] = s4
	sw		s5,80(a0)		# reg[20] = s5
	sw		s6,84(a0)		# reg[21] = s6
	sw		s7,88(a0)		# reg[22] = s7
	sw		s8,92(a0)		# reg[23] = s8
	sw		s9,96(a0)		# reg[24] = s9
    sw		s10,100(a0)	# reg[25] = s10
	sw		s11,104(a0)	# reg[26] = s11
	#sw		t3,108(a0)		# reg[27] = t3
	#sw		t4,112(a0)		# reg[28] = t4
	#sw		t5,116(a0)		# reg[29] = t5
	#sw		t6,120(a0)		# reg[30] = t6
	#sw		pc,124(a0)		# TODO: SAVE EPC but keep commented!

	jr		ra

###################################################

.globl  ASM_RunScheduledTask
ASM_RunScheduledTask:

	mv		t6,a0

	lw		ra,0(t6)		# reg[0] = ra
	lw		sp,4(t6)		# reg[1] = sp
	lw		gp,8(t6)		# reg[2] = gp
	lw		tp,12(t6)		# reg[3] = tp
	lw		t0,16(t6)		# reg[4] = t0
	lw		t1,20(t6)		# reg[5] = t1
	lw		t2,24(t6)		# reg[6] = t2
	lw		s0,28(t6)		# reg[7] = s0
	lw		s1,32(t6)		# reg[8] = s1
	lw		t6,36(t6)		# reg[9] = t6
	lw		a1,40(t6)		# reg[10] = a1
	lw		a2,44(t6)		# reg[11] = a2
	lw		a3,48(t6)		# reg[12] = a3
	lw		a4,52(t6)		# reg[13] = a4
	lw		a5,56(t6)		# reg[14] = a5
	lw		a6,60(t6)		# reg[15] = a6
	lw		a7,64(t6)		# reg[16] = a7
	lw		s2,68(t6)		# reg[17] = s2
	lw		s3,72(t6)		# reg[18] = s3
	lw		s4,76(t6)		# reg[19] = s4
	lw		s5,80(t6)		# reg[20] = s5
	lw		s6,84(t6)		# reg[21] = s6
	lw		s7,88(t6)		# reg[22] = s7
	lw		s8,92(t6)		# reg[23] = s8
	lw		s9,96(t6)		# reg[24] = s9
    lw		s10,100(t6)		# reg[25] = s10
	lw		s11,104(t6)		# reg[26] = s11
	lw		t3,108(t6)		# reg[27] = t3
	lw		t4,112(t6)		# reg[28] = t4
	lw		t5,116(t6)		# reg[29] = t5
	lw		t6,120(t6)		# reg[30] = t6
	#lw		pc,124(t6)		# TODO: loads pc of the task that will run
	#lw		t6,128(t6)		# TODO: loads offset of the task (for paging setup)

	#li		$1,0x1
	#mtc0	t6,$10			# TODO: load offset of task to c0's EntryHi
	#mtc0	$1,$12			# writes 1 to c0's SR, enabling interrupts
	
###################################################

.globl  ASM_SetInterruptEnable        
ASM_SetInterruptEnable:
	#mv		t6, a0
	#mfc0	a0,$12
	#mtc0	t6,$12          # TODO: enable interrupts
	jr		ra

################## READ_INST ################## 

.globl  read_inst
read_inst:        
	la		t6,global_inst

	#mfc0	t5,$16
	sw		t5,0(t6)		# TODO: save global_inst

	jr		ra				# return to the caller 
