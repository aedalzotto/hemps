##------------------------------------------------------------------------------------------------
##
##  DISTRIBUTED HEMPS -  5.0
##
##  Research group: GAPH-PUCRS    -    contact   fernando.moraes@pucrs.br
##
##  Distribution:  September 2013
##
##  Source name:  boot.S
##
##  Brief description: Initializes the stack pointer and jumps to main().
##
##------------------------------------------------------------------------------------------------

        .text      
        .align  2
        .set noat
        .globl  entry
        .ent    entry
entry:
        .set noreorder
        
		li $sp,sp_addr
		jal     main
		nop

$L1:
        j $L1
        nop

        .org 0x3c
#address 0x3c
interrupt_service_routine_entry:  
        j       interrupt_service_routine
        nop

#address 0x44
system_service_routine_entry:  
        j       system_service_routine 
        nop

#address 0x4c
interrupt_service_routine:

        #REGISTERS $26 AND $27 ARE RESERVED FOR THE OS  
        #la      $26,needTaskScheduling
        #sw      $0,0($26)       

        #SAVE ra AND REGISTERS
        la      $27,current
        lw      $27,0($27)
                                   
        sw      $2,8($27)       # reg[2] = $v0
        sw      $3,12($27)       # reg[3] = $v1
        sw      $4,16($27)       # reg[4] = $a0
        sw      $5,20($27)      # reg[5] = $a1
        sw      $6,24($27)      # reg[6] = $a2
        sw      $7,28($27)      # reg[7] = $a3
        sw      $8,32($27)      # reg[8] = $t0
        sw      $9,36($27)      # reg[9] = $t1
        sw      $10,40($27)     # reg[10] = $t2
        sw      $11,44($27)     # reg[11] = $t3
        sw      $12,48($27)     # reg[12] = $t4
        sw      $13,52($27)     # reg[13] = $t5
        sw      $14,56($27)     # reg[14] = $t6
        sw      $15,60($27)     # reg[15] = $t7
        sw      $16,64($27)     # reg[16] = $s0
        sw      $17,68($27)     # reg[17] = $s1
        sw      $18,72($27)     # reg[18] = $s2
        sw      $19,76($27)     # reg[19] = $s3
        sw      $20,80($27)     # reg[20] = $s4
        sw      $21,84($27)     # reg[21] = $s5
        sw      $22,88($27)     # reg[22] = $s6
        sw      $23,92($27)     # reg[23] = $s7
        sw      $24,96($27)     # reg[24] = $t8
        sw      $25,100($27)    # reg[25] = $t9
        # reg[26] = k0
        # reg[27] = k1
#        sw      $28,112($27)     # reg[28] = $gp
        sw      $29,116($27)    # reg[29] = $sp
        sw      $30,120($27)    # reg[30] = $s8
        sw      $31,124($27)    # reg[31] = $ra

        mfhi    $26
        sw      $26,104($27)    # reg[26] = $hi
        mflo    $26
        sw      $26,108($27)    # reg[27] = $lo

        mfc0    $26,$14
        sw      $26,128($27)    # save PC


        li $sp,sp_addr
        
        #JUMP TO INTERRUPT SERVICE ROUTINE
        lui     $5,0x2000    
        lw      $4,0x20($5)     #IRQ_STATUS
        lw      $6,0x10($5)     #IRQ_MASK
        jal     OS_InterruptServiceRoutine
        and     $4,$4,$6

####################################################

system_service_routine:
        # registers $26 and $27 are reserved for kernel       
        la      $27,current
        lw      $27,0($27)
        
        # saves minimun context
        sw      $4,16($27)      # $a0
        sw      $5,20($27)      # $a1
        sw      $6,24($27)      # $a2
        sw      $7,28($27)      # $a3        

#        sw      $28,96($27)     # $gp
        sw      $29,116($27)    # $sp  
        sw      $31,124($27)    # $ra
        
        mfc0    $26,$14                 
        sw      $26,128($27)    # save PC                


        li $sp,sp_addr

        # jumps to system calls handler
        jal     Syscall
        nop
        
        
        # verifies if scheduling is needed
        la      $26,schedule_after_syscall
        lw      $26,0($26)
        beqz    $26,system_service_restore
        nop
        
        # saves the context of the running task
        la      $4,current
        lw      $4,0($4)
        jal     ASM_SaveRemainingContext
        nop
        
        # schedules the next ready task
        jal     Scheduler
        nop
        
        # restores the context and runs the scheduled task
        la      $4,current
        lw      $4,0($4)
        jal     ASM_RunScheduledTask        
        nop
       
system_service_restore:

        la      $27,current
        lw      $27,0($27)        
        
        # restores the context previously saved
#        lw      $28,96($27)     # $gp
        lw      $29,116($27)    # $sp  
        lw      $31,124($27)    # $ra
        
        lw      $26,128($27)    # loads pc of the task that was running
        lw      $27,132($27)    # loads offset of the task (for paging setup)
        li      $1,0x1  
        
        mtc0    $27,$10
        jr      $26             # jumps to pc
        mtc0    $1,$12          # enables interrupts    

        .set    reorder 
        .end    entry
        
###################################################
        
        .globl  ASM_SaveRemainingContext        
        .ent    ASM_SaveRemainingContext
        .set    noreorder
        
ASM_SaveRemainingContext:

        sw      $2,8($4)        #  $v0  
        sw      $3,12($4)        #  $v1
        #sw      $8,32($4)       #  $t0
        #sw      $9,36($4)       #  $t1
        #sw      $10,40($4)      #  $t2  
        #sw      $11,44($4)      #  $t3
        #sw      $12,48($4)      #  $t4
        #sw      $13,52($4)      #  $t5
        #sw      $14,56($4)      #  $t6
        #sw      $15,60($4)      #  $t7
        sw      $16,64($4)      #  $s0  
        sw      $17,68($4)      #  $s1
        sw      $18,72($4)      #  $s2
        sw      $19,76($4)      #  $s3
        sw      $20,80($4)      #  $s4
        sw      $21,84($4)      #  $s5
        sw      $22,88($4)      #  $s6
        sw      $23,92($4)      #  $s7
        #sw      $24,96($4)      #  $t8
        #sw      $25,100($4)      #  $t9
        sw      $30,120($4)     #  $s8
        #mfhi    $26
        #sw      $26,104($4)     #  $hi
        #mflo    $26
        #sw      $26,108($4)     #  $lo
        
        #mfc0    $26,$14                 
        #sw      $26,128($4)     #  save PC
        
        jr      $31
        nop
        
        .set    reorder
        .end    ASM_SaveRemainingContext
        
###################################################

        .globl  ASM_RunScheduledTask
        .ent    ASM_RunScheduledTask
        .set    noreorder
        
ASM_RunScheduledTask:
        
        move    $27,$4
        
        lw      $2,8($27)       # $v0  
        lw      $3,12($27)       # $v1
        lw      $4,16($27)       # $a0
        lw      $5,20($27)      # $a1
        lw      $6,24($27)      # $a2
        lw      $7,28($27)      # $a3
        lw      $8,32($27)      # $t0
        lw      $9,36($27)      # $t1
        lw      $10,40($27)     # $t2  
        lw      $11,44($27)     # $t3
        lw      $12,48($27)     # $t4
        lw      $13,52($27)     # $t5
        lw      $14,56($27)     # $t6
        lw      $15,60($27)     # $t7
        lw      $16,64($27)     # $s0  
        lw      $17,68($27)     # $s1
        lw      $18,72($27)     # $s2
        lw      $19,76($27)     # $s3
        lw      $20,80($27)     # $s4
        lw      $21,84($27)     # $s5
        lw      $22,88($27)     # $s6
        lw      $23,92($27)     # $s7
        lw      $24,96($27)     # $t8
        lw      $25,100($27)     # $t9
#        lw      $28,112($27)     # $gp
        lw      $29,116($27)    # $sp  
        lw      $30,120($27)    # $s8
        lw      $31,124($27)    # $ra  
        lw      $26,104($27)    # $hi
        mthi    $26
        lw      $26,108($27)    # $lo
        mtlo    $26

        lw      $26,128($27)    # loads pc of the task that will run
        lw      $27,132($27)    # loads offset of the task (for paging setup)
        li      $1,0x1  

        mtc0    $27,$10
        jr      $26             # jumps to pc
        mtc0    $1,$12          # enables interrupts
        
        .set    reorder
        .end    ASM_RunScheduledTask
        
###################################################

        .globl  ASM_SetInterruptEnable
        .ent    ASM_SetInterruptEnable
        
ASM_SetInterruptEnable:
        .set    noreorder
        mfc0    $2,$12
        jr      $31
        mtc0    $4,$12          #enable interrupts

        .set    reorder
        .end    ASM_SetInterruptEnable

################## READ_INST ################## 

        .globl  read_inst
        .ent    read_inst
        .set    noreorder
read_inst:        
        la      $27,global_inst
                                   
        mfc0    $26,$16                 
        sw      $26,0($27)    #  save global_inst

        jr      $31             # return to the caller 
        nop 
        
        .set    reorder 
        .end    read_inst
