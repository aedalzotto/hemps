/**
 * @file riscv.cpp
 * 
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * UPF - Universidade de Passo Fundo (upf.br)
 * 
 * @brief
 * Source file for a generic RISC-V CPU ISS running a RV32I ISA
 * with M/S/U privileges.
 */

#include "riscv.h"

#ifdef MTI_SYSTEMC
SC_MODULE_EXPORT(RiscV);
#endif

RiscV::RiscV(sc_module_name name_, half_flit_t router_addr_) : 
				sc_module(name_), router_addr(router_addr_)
{
	SC_THREAD(cpu);
	sensitive << clock.pos() << mem_pause.pos();
	sensitive << mem_pause.neg();
}

void RiscV::cpu()
{

	while(true) {

		if(reset_in.read()){
			reset();
		}

		sc_uint<XLEN> instr = fetch();

	}

}

void RiscV::reset()
{
	pc = 0;
	for(int i = 0; i < 32; i++)
		x[i] = 0;
}

sc_uint<XLEN> fetch()
{
	sc_uint<XLEN> ppc = vatp(pc);
	// Step 1.
	sc_uint<XLEN> a = satp::PPN() * PAGESIZE::Sv32;
	sc_int<XLEN> i = LEVELS::Sv32 - 1;

	// Step 2.
	sc_uint<XLEN> pteaddr = a + va::VPN(i) * PTESIZE::Sv32;
	sc_uint<XLEN> pte = 
	
}


void Rv32i::get_opcode()
{
	mem_address.write(state->pc); // Read opcode.
	opcode = mem_data_r.read();
	wait(1);
}

void Rv32i::reset_execution()
{
	page 		= 0;
	state->pc 	= 0;
	global_inst = 0;
	intr_en 	= false;
	prefetch  	= false;
	jmp_or_br 	= false;
	mem_byte_we.write(0x0);  		// ?
	mem_address.write(state->pc);   // 0
	word_addr = -4;					// 0xFFFFFFFC ??
	wait(17);
}

void Rv32i::interrupt()
{
	state->epc = state->pc - 4; // Exception PC 
	page = 0;		  			// OS Page
	state->pc = 0x3C; 			// Interrupt vector. JMP to INTERRUPT_SERVICE_ROUTINE in Boot.S
								// It saves context and then jumps to OS_InterruptServiceRoutine in kernel_slave.c
	get_opcode();			
	intr_enable = false;
}

void Rv32i::read_opcode()
{
	// Instruction read.
	mem_address.write(state->pc);
	if(prefetch) {
		opcode = prefetched_opcode;
		prefetch = false;
	}
	else
		instr = mem_data_r.read();

	opcode = instr & 0x7f; // instr[6:0]
	/*
	rs = (instr >> 21) & 0x1f;
	rt = (instr >> 16) & 0x1f;
	rd = (instr >> 11) & 0x1f;
	re = (instr >>  6) & 0x1f;
	func = instr & 0x3f;
	imm = instr & 0xffff;
	imm_shift = (((int)(short)imm) << 2) - 4;
	target = (instr << 6) >> 4;
	ptr = (short)imm + r[rs];
	ptr |= page;	// Adds the page number.
	*/

	x[0] = 0; // Reassures x[0] is 0
	jump_or_branch = false; // Still not decoded
}


void Rv32i::rv32i_execution() {
	
	// Runs forever
	while(true){

		// Current page in execution in the port.
		current_page.write(page>>shift);

		if(!mem_pause)
			pc_last = state->pc;	// Stores the last pc address

		if(reset.read()){ // On reset
			reset_execution();
			continue;  	  // Run the cpu thread again
		}

		// On execution

		r = (int*)state->r;				// Signed mask.   ???
		u = (unsigned int*)state->r;	// Unsigned mask. ???

		// Control transfer instructions in RV32I do not have architecturally visible delay slots
		if(intr_enable && intr_in.read()){ // Check interruption if enabled
			interrupt();
			continue;					// Executes the loaded instruction after the ISR.
		} else {
			state->pc += 4;  // Next instruction
		}

		// Adds the page number to the PC. Relative addressing.
		state->pc |= page;

		read_opcode();

/*
		if (instr == 0) { // NOP
			nop_inst_kernel = (page != 0? nop_inst_kernel     : nop_inst_kernel + 1 );
			nop_inst_tasks  = (page != 0? nop_inst_tasks + 1  : nop_inst_tasks );
			wait(1);
			continue; // Next instruction
		}
*/
		pc_count = state->pc; // ?

		// Instruction decode, execution and write back.
		switch(opcode) {
			case OP:	// R-type
				rd = (instr >> 7) & 0x


// MIPS
			case 0x00:/*SPECIAL*/
				switch(func) {
					case 0x00:/*SLL*/
						wait(1);
						r[rd]=r[rt]<<re;
						
						shift_inst_kernel=(page != 0? shift_inst_kernel : shift_inst_kernel + 1 );
						shift_inst_tasks=(page != 0? shift_inst_tasks + 1  : shift_inst_tasks );
							
							
					break;

					case 0x02:/*SRL*/
						wait(1);
						r[rd]=u[rt]>>re;
						
						shift_inst_kernel=(page != 0? shift_inst_kernel : shift_inst_kernel + 1 );
						shift_inst_tasks=(page != 0? shift_inst_tasks + 1  : shift_inst_tasks );

					break;

					case 0x03:/*SRA*/
						wait(1);
						r[rd]=r[rt]>>re;
						
						shift_inst_kernel=(page != 0? shift_inst_kernel : shift_inst_kernel + 1 );
						shift_inst_tasks=(page != 0? shift_inst_tasks + 1  : shift_inst_tasks );

					break;

					case 0x04:/*SLLV*/
						wait(1);
						r[rd]=r[rt]<<r[rs];
						
						shift_inst_kernel=(page != 0? shift_inst_kernel : shift_inst_kernel + 1 );
						shift_inst_tasks=(page != 0? shift_inst_tasks + 1  : shift_inst_tasks );

					break;

					case 0x06:/*SRLV*/
						wait(1);
						r[rd]=u[rt]>>r[rs];
						
						shift_inst_kernel=(page != 0? shift_inst_kernel : shift_inst_kernel + 1 );
						shift_inst_tasks=(page != 0? shift_inst_tasks + 1  : shift_inst_tasks );

					break;

					case 0x07:/*SRAV*/
						wait(1);
						r[rd]=r[rt]>>r[rs];
						
						shift_inst_kernel=(page != 0? shift_inst_kernel : shift_inst_kernel + 1 );
						shift_inst_tasks=(page != 0? shift_inst_tasks + 1  : shift_inst_tasks );

					break;

					case 0x08:/*JR*/
						jump_or_branch = true;
						state->pc = r[rs];
						state->pc |= page;
						mem_address.write(state->pc);
						wait(1);
						
						jump_inst_kernel=(page != 0? jump_inst_kernel : jump_inst_kernel + 1 );
						jump_inst_tasks=(page != 0? jump_inst_tasks + 1  : jump_inst_tasks );
						
					break;

					case 0x09:/*JALR*/
						jump_or_branch = true;
						r[rd] = state->pc;
						state->pc = r[rs];
						state->pc |= page;
						mem_address.write(state->pc);
						wait(1);
						
						jump_inst_kernel=(page != 0? jump_inst_kernel : jump_inst_kernel + 1 );
						jump_inst_tasks=(page != 0? jump_inst_tasks + 1  : jump_inst_tasks );

					break;

					case 0x0a:/*MOVZ*/
						wait(1);
						if ( !r[rt] )
							r[rd] = r[rs];
							
						move_inst_kernel=(page != 0? move_inst_kernel : move_inst_kernel + 1 );
						move_inst_tasks=(page != 0? move_inst_tasks + 1  : move_inst_tasks );

					break;  /*IV*/

					case 0x0b:/*MOVN*/
						wait(1);
						if ( r[rt] )
							r[rd] = r[rs];
							
						move_inst_kMODULEernel=(page != 0? move_inst_kernel : move_inst_kernel + 1 );
						move_inst_tasks=(page != 0? move_inst_tasks + 1  : move_inst_tasks );

					break;  /*IV*/

					case 0x0c:/*SYSCALL*/
						state->epc = state->pc;
						state->pc = 0x44;							
						page = 0;
						current_page.write(page>>shift);
						mem_address.write(state->pc);
						wait(1);
						intr_enable = false;
									
						other_inst_kernel=(page != 0? other_inst_kernel : other_inst_kernel + 1 );
						other_inst_tasks=(page != 0? other_inst_tasks + 1  : other_inst_tasks );
						
					break;

					case 0x0d:/*BREAK*/
						wait(1);
						
						other_inst_kernel=(page != 0? other_inst_kernel : other_inst_kernel + 1 );
						other_inst_tasks=(page != 0? other_inst_tasks + 1  : other_inst_tasks );

					break;

					case 0x0f:/*SYNC*/
						wait(1);
						
						other_inst_kernel=(page != 0? other_inst_kernel : other_inst_kernel + 1 );
						other_inst_tasks=(page != 0? other_inst_tasks + 1  : other_inst_tasks );

					break;

					case 0x10:/*MFHI*/
						wait(1);
						r[rd] = state->hi;
						
						move_inst_kernel=(page != 0? move_inst_kernel : move_inst_kernel + 1 );
						move_inst_tasks=(page != 0? move_inst_tasks + 1  : move_inst_tasks );

					break;

					case 0x11:/*MTHI*/
						wait(1);
						state->hi = r[rs];
						
						move_inst_kernel=(page != 0? move_inst_kernel : move_inst_kernel + 1 );
						move_inst_tasks=(page != 0? move_inst_tasks + 1  : move_inst_tasks );

					break;

					case 0x12:/*MFLO*/
						wait(1);
						r[rd] = state->lo;
						
						move_inst_kernel=(page != 0? move_inst_kernel : move_inst_kernel + 1 );
						move_inst_tasks=(page != 0? move_inst_tasks + 1  : move_inst_tasks );

					break;

					case 0x13:/*MTLO*/
						wait(1);
						state->lo = r[rs];
						
						move_inst_kernel=(page != 0? move_inst_kernel : move_inst_kernel + 1 );
						move_inst_tasks=(page != 0? move_inst_tasks + 1  : move_inst_tasks );
						
					break;

					case 0x18:/*MULT*/
						wait(1);

						prefetch = true;
						prefetched_opcode = mem_data_r.read();
						mem_address.write(state->pc);
						wait(4);

						mult_big_signed(r[rs],r[rt]);
						//sc_int<64> result;
						//result = r[rs] * r[rt];
						//state->hi = result.range(63,32);
						//state->lo = result.range(31,0);
						
						mult_div_inst_kernel=(page != 0? mult_div_inst_kernel : mult_div_inst_kernel + 1 );
						mult_div_inst_tasks=(page != 0? mult_div_inst_tasks + 1  : mult_div_inst_tasks );

					break;

					case 0x19:/*MULTU*/
						//state->lo=r[rs]*r[rt]; state->hi=0; break;
						wait(1);

						prefetch = true;
						prefetched_opcode = mem_data_r.read();
						mem_address.write(state->pc);
						wait(4);

						mult_big(r[rs],r[rt]);
						//sc_uint<64> result;
						//result = u[rs] * u[rt];
						//state->hi = result.range(63,32);
						//state->lo = result.range(31,0);
						
						mult_div_inst_kernel=(page != 0? mult_div_inst_kernel : mult_div_inst_kernel + 1 );
						mult_div_inst_tasks=(page != 0? mult_div_inst_tasks + 1  : mult_div_inst_tasks );

					break;

					case 0x1a:/*DIV*/
						wait(1);

						prefetch = true;
						prefetched_opcode = mem_data_r.read();
						mem_address.write(state->pc);
						wait(4);
						
						state->lo = (r[rt]>0) ? r[rs] / r[rt] : 0;
						state->hi = (r[rt]>0) ? r[rs] % r[rt] : r[rs];
				//		state->lo = r[rs] / r[rt];						
				//		state->hi = r[rs] % r[rt];
				
						mult_div_inst_kernel=(page != 0? mult_div_inst_kernel : mult_div_inst_kernel + 1 );
						mult_div_inst_tasks=(page != 0? mult_div_inst_tasks + 1  : mult_div_inst_tasks );
						
					break;

					case 0x1b:/*DIVU*/
						wait(1);

						prefetch = true;
						prefetched_opcode = mem_data_r.read();
						mem_address.write(state->pc);
						wait(4);

						state->lo = u[rs] / u[rt];
						state->hi = u[rs] % u[rt];
						
						mult_div_inst_kernel=(page != 0? mult_div_inst_kernel : mult_div_inst_kernel + 1 );
						mult_div_inst_tasks=(page != 0? mult_div_inst_tasks + 1  : mult_div_inst_tasks );

					break;

					case 0x20:/*ADD*/
						wait(1);
						r[rd] = r[rs] + r[rt];
						
						arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
						arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );

					break;

					case 0x21:/*ADDU*/
						wait(1);
						r[rd] = r[rs] + r[rt];
						
						arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
						arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );

					break;

					case 0x22:/*SUB*/
						wait(1);
						r[rd] = r[rs] - r[rt];
						
						arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
						arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );

					break;

					case 0x23:/*SUBU*/
						wait(1);
						r[rd] = r[rs] - r[rt];
						
						arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
						arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );

					break;

					case 0x24:/*AND*/
						wait(1);
						r[rd] = r[rs] & r[rt];
						
						logical_inst_kernel=(page != 0? logical_inst_kernel : logical_inst_kernel + 1 );
						logical_inst_tasks=(page != 0? logical_inst_tasks + 1  : logical_inst_tasks );
						
					break;

					case 0x25:/*OR*/
						wait(1);
						r[rd] = r[rs] | r[rt];
						
						logical_inst_kernel=(page != 0? logical_inst_kernel : logical_inst_kernel + 1 );
						logical_inst_tasks=(page != 0? logical_inst_tasks + 1  : logical_inst_tasks );

					break;

					case 0x26:/*XOR*/
						wait(1);
						r[rd] = r[rs] ^ r[rt];
						
						logical_inst_kernel=(page != 0? logical_inst_kernel : logical_inst_kernel + 1 );
						logical_inst_tasks=(page != 0? logical_inst_tasks + 1  : logical_inst_tasks );

					break;

					case 0x27:/*NOR*/
						wait(1);
						r[rd] = ~(r[rs] | r[rt]);
						
						logical_inst_kernel=(page != 0? logical_inst_kernel : logical_inst_kernel + 1 );
						logical_inst_tasks=(page != 0? logical_inst_tasks + 1  : logical_inst_tasks );

					break;

					case 0x2a:/*SLT*/
						wait(1);
						r[rd]= (r[rs] < r[rt]);
						
						arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
						arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );

					break;

					case 0x2b:/*SLTU*/
						wait(1);
						r[rd] = (u[rs] < u[rt]);
						
						arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
						arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );

					break;

					case 0x2d:/*DADDU*/
						wait(1);
						r[rd] = r[rs] + u[rt];
						
						arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
						arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );

					break;

					case 0x31:/*TGEU*/ 
					
						wait(1); 
						other_inst_kernel=(page != 0? other_inst_kernel : other_inst_kernel + 1 );
						other_inst_tasks=(page != 0? other_inst_tasks + 1  : other_inst_tasks );

					break;
					
					case 0x32:/*TLT*/  
					
						wait(1); 
						other_inst_kernel=(page != 0? other_inst_kernel : other_inst_kernel + 1 );
						other_inst_tasks=(page != 0? other_inst_tasks + 1  : other_inst_tasks );
						
					break;
					
					case 0x33:/*TLTU*/ 
					
						wait(1); 
						other_inst_kernel=(page != 0? other_inst_kernel : other_inst_kernel + 1 );
						other_inst_tasks=(page != 0? other_inst_tasks + 1  : other_inst_tasks );
						
					break;
					case 0x34:/*TEQ*/  
					
						wait(1); 
						other_inst_kernel=(page != 0? other_inst_kernel : other_inst_kernel + 1 );
						other_inst_tasks=(page != 0? other_inst_tasks + 1  : other_inst_tasks );

					break;
					case 0x36:/*TNE*/ 
						
						wait(1); 
						other_inst_kernel=(page != 0? other_inst_kernel : other_inst_kernel + 1 );
						other_inst_tasks=(page != 0? other_inst_tasks + 1  : other_inst_tasks );

					break;
					default:
						printf("\nPE:%x ERROR0 address=%lu opcode=%u\n",(int) address_router, state->pc,opcode);
						return;
				}
			break;

			case 0x01:/*REGIMM*/
				switch(rt) {
					case 0x10:/*BLTZAL*/
						if ( r[rs] < 0 ) {
							jump_or_branch = true;
							r[31] = state->pc;
							state->pc += imm_shift;
							mem_address.write(state->pc);
						}
						wait(1);
						
						branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
						branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

					break;

					case 0x00:/*BLTZ*/
						if ( r[rs] < 0 ) {
							jump_or_branch = true;
							state->pc += imm_shift;
							mem_address.write(state->pc);
						}
						wait(1);
						
						branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
						branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

					break;

					case 0x11:/*BGEZAL*/
						if ( r[rs] >= 0 ) {
							jump_or_branch = true;
							r[31] = state->pc;
							state->pc += imm_shift;
							mem_address.write(state->pc);
						}
						wait(1);
						
						branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
						branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

					break;

					case 0x01:/*BGEZ*/
						if ( r[rs] >= 0 ) {
							jump_or_branch = true;
							state->pc += imm_shift;
							mem_address.write(state->pc);
						}
						wait(1);
						
						branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
						branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

					break;

					case 0x12:/*BLTZALL*/
						if ( r[rs] < 0 ) {
							jump_or_branch = true;
							r[31] = state->pc;
							state->pc += imm_shift;
							mem_address.write(state->pc);
						}
						wait(1);
						
						branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
						branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

					break;

					case 0x02:/*BLTZL*/
						if ( r[rs] < 0 ) {
							jump_or_branch = true;
							state->pc += imm_shift;
							mem_address.write(state->pc);
						}
						wait(1);
						
						branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
						branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

					break;

					case 0x13:/*BGEZALL*/
						if ( r[rs] >= 0 ) {
							jump_or_branch = true;
							r[31] = state->pc;
							state->pc += imm_shift;
							mem_address.write(state->pc);
						}
						wait(1);
						
						branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
						branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

					break;

					case 0x03:/*BGEZL*/
						if ( r[rs] >= 0 ) {
							jump_or_branch = true;
							state->pc += imm_shift;
							mem_address.write(state->pc);
						}
						wait(1);
						
						branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
						branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

					break;

					default:
						printf("\nPE: %x ERROR1 address=%lu opcode=%u\n",(int)address_router, state->pc,opcode);
						return;
				}
			break;

			case 0x03:/*JAL*/
				jump_or_branch = true;
				r[31] = state->pc;
				state->pc = (state->pc & 0xf0000000) | target;
				state->pc |= page;				// Adds the page number.
				mem_address.write(state->pc);
				wait(1);
				
				jump_inst_kernel=(page != 0? jump_inst_kernel : jump_inst_kernel + 1 );
				jump_inst_tasks=(page != 0? jump_inst_tasks + 1  : jump_inst_tasks );

			break;

			case 0x02:/*J*/
				jump_or_branch = true;
				state->pc = (state->pc & 0xf0000000) | target;
				state->pc |= page;				// Adds the page number.
				mem_address.write(state->pc);
				wait(1);
				
				jump_inst_kernel=(page != 0? jump_inst_kernel : jump_inst_kernel + 1 );
				jump_inst_tasks=(page != 0? jump_inst_tasks + 1  : jump_inst_tasks );

			break;

			case 0x04:/*BEQ*/
				if ( r[rs] == r[rt] ) {
					jump_or_branch = true;
					state->pc += imm_shift;
					mem_address.write(state->pc);
				}
				wait(1);
				
				branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
				branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

			break;

			case 0x05:/*BNE*/
				if ( r[rs] != r[rt] ) {
					jump_or_branch = true;
					state->pc += imm_shift;
					mem_address.write(state->pc);
				}
				wait(1);
				
				branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
				branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

			break;

			case 0x06:/*BLEZ*/
				if ( r[rs] <= 0 ) {
					jump_or_branch = true;
					state->pc += imm_shift;
					mem_address.write(state->pc);
				}
				wait(1);
				
				branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
				branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

			break;

			case 0x07:/*BGTZ*/
				if ( r[rs] > 0 ) {
					jump_or_branch = true;
					state->pc += imm_shift;
					mem_address.write(state->pc);
				}
				wait(1);
				
				branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
				branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

			break;

			case 0x08:/*ADDI*/
				wait(1);
				r[rt] = r[rs] + (short)imm;
				
				arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
				arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );
				
			break;

			case 0x09:/*ADDIU*/
				wait(1);
				u[rt] = u[rs] + (short)imm;
				
				arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
				arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );

			break;

			case 0x0a:/*SLTI*/
				wait(1);
				r[rt] = r[rs] < (short)imm;
				
				arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
				arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );

			break;

			case 0x0b:/*SLTIU*/
				wait(1);
				u[rt] = u[rs] < (unsigned int)(short)imm;
				
				arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
				arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );

			break;

			case 0x0c:/*ANDI*/
				wait(1);
				r[rt] = r[rs] & imm;
				
				logical_inst_kernel=(page != 0? logical_inst_kernel : logical_inst_kernel + 1 );
				logical_inst_tasks=(page != 0? logical_inst_tasks + 1  : logical_inst_tasks );
				
			break;

			case 0x0d:/*ORI*/
				wait(1);
				r[rt] = r[rs] | imm;
				
				logical_inst_kernel=(page != 0? logical_inst_kernel : logical_inst_kernel + 1 );
				logical_inst_tasks=(page != 0? logical_inst_tasks + 1  : logical_inst_tasks );

			break;

			case 0x0e:/*XORI*/
				wait(1);
				r[rt] = r[rs] ^ imm;
				
				logical_inst_kernel=(page != 0? logical_inst_kernel : logical_inst_kernel + 1 );
				logical_inst_tasks=(page != 0? logical_inst_tasks + 1  : logical_inst_tasks );


			break;

			case 0x0f:/*LUI*/
				wait(1);
				r[rt] = (imm<<16);
				
				arith_inst_kernel=(page != 0? arith_inst_kernel : arith_inst_kernel + 1 );
				arith_inst_tasks=(page != 0? arith_inst_tasks + 1  : arith_inst_tasks );

			break;

			case 0x10:/*COP0*/
				wait(1);
				move_inst_kernel=(page != 0? move_inst_kernel : move_inst_kernel + 1 );	
				move_inst_tasks=(page != 0? move_inst_tasks + 1  : move_inst_tasks );
						
				if ( opcode & (1<<23) )	{/*MTC0*/
					switch (rd) {
						case 10:
							page = r[rt];
						break;

						case 12:
							intr_enable = r[rt] ;
						break;

						case 14:
							state->epc = r[rt];
						break;							
						
						case 16:
							r[rt] = global_inst;
						break;

						default:
							printf("MTC0: reg %d not mapped.\n",rd);
							return;
					}
				}
				else { /*MFC0*/
					switch (rd) {
						case 10:
							r[rt] = page;
						break;

						case 12:
							r[rt] = intr_enable;
						break;

						case 14:
							r[rt] = state->epc;
						break;
						
						case 16:
							r[rt] = global_inst;
						break;
						
						default:
							printf("MFC0: reg %d not mapped.\n",rd);
							return;
					}
				}
			break;

	//      case 0x11:/*COP1*/ break;
	//      case 0x12:/*COP2*/ break;
	//      case 0x13:/*COP3*/ break;

			case 0x14:/*BEQL*/
				if ( r[rs] == r[rt] ) {
					jump_or_branch = true;
					state->pc += imm_shift;
					mem_address.write(state->pc);
				}
				wait(1);
				
				branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
				branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

			break;

			case 0x15:/*BNEL*/
				if ( r[rs] != r[rt] ) {
					jump_or_branch = true;
					state->pc += imm_shift;
					mem_address.write(state->pc);
				}
				wait(1);
				
				branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
				branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

			break;

			case 0x16:/*BLEZL*/
				if ( r[rs] <= 0 ) {
					jump_or_branch = true;
					state->pc += imm_shift;
					mem_address.write(state->pc);
				}
				wait(1);
				
				branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
				branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

			break;

			case 0x17:/*BGTZL*/
				if ( r[rs] > 0 ) {
					jump_or_branch = true;
					state->pc += imm_shift;
					mem_address.write(state->pc);
				}
				wait(1);
				
				branch_inst_kernel=(page != 0? branch_inst_kernel : branch_inst_kernel + 1 );
				branch_inst_tasks=(page != 0? branch_inst_tasks + 1  : branch_inst_tasks );

			break;

	//      case 0x1c:/*MAD*/  break;   /*IV*/

			case 0x20:/*LB*/
				mem_address.write(ptr & word_addr);	// Address the memory with word address.
				wait(1);
				
				load_inst_kernel=(page != 0? load_inst_kernel : load_inst_kernel + 1 );
				load_inst_tasks=(page != 0? load_inst_tasks + 1  : load_inst_tasks );

				// Verifies the mem_pause signal at the first execution cycle
				if ( mem_pause.read() ) {
					mem_address.write(pc_last);	// Keep the last memory address before mem_pause = '1'

					while (mem_pause.read())	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(ptr & word_addr);// Address the memory with word address.
					wait(1);
				}

				prefetch = true;
				prefetched_opcode = mem_data_r.read();
				mem_address.write(state->pc);
				wait(1);

				// Verifies the mem_pause signal at the second execution cycle
				if (mem_pause.read()) {
					mem_address.write(ptr & word_addr);// Keep the last memory address before mem_pause = '1'

					while (mem_pause.read())	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(state->pc);// Address the next instruction
					wait(1);
				}

				if ((ptr & 3) == 3)
					if (big_endian)
						r[rt] = (char)mem_data_r.read().range(7,0);
					else
						r[rt] = (char)mem_data_r.read().range(31,24);

				else if ((ptr & 2) == 2)
					if (big_endian)
						r[rt] = (char)mem_data_r.read().range(15,8);
					else
						r[rt] = (char)mem_data_r.read().range(23,16);

				else if ((ptr & 1) == 1)
					if (big_endian)
						r[rt] = (char)mem_data_r.read().range(23,16);
					else
						r[rt] = (char)mem_data_r.read().range(15,8);

				else
					if (big_endian)
						r[rt] = (char)mem_data_r.read().range(31,24);
					else
						r[rt] = (char)mem_data_r.read().range(7,0);


			break;

			case 0x21:/*LH*/
				//assert((ptr & 1) == 0);
				mem_address.write(ptr & word_addr);	// Address the memory with word address.
				wait(1);
				load_inst_kernel=(page != 0? load_inst_kernel : load_inst_kernel + 1 );
				load_inst_tasks=(page != 0? load_inst_tasks + 1  : load_inst_tasks );

				// Verifies the mem_pause signal at the first execution cycle
				if ( mem_pause.read() ) {
					mem_address.write(pc_last);	// Keep the last memory address before mem_pause = '1'

					while (mem_pause.read())	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(ptr & word_addr);// Address the memory with word address.
					wait(1);
				}

				prefetch = true;
				prefetched_opcode = mem_data_r.read();
				mem_address.write(state->pc);
				wait(1);

				// Verifies the mem_pause signal at the second execution cycle
				if (mem_pause.read()) {
					mem_address.write(ptr & word_addr);// Keep the last memory address before mem_pause = '1'

					while (mem_pause.read())	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(state->pc);// Address the next instruction
					wait(1);
				}

				if ((ptr & 2) == 2)
					if (big_endian)
						r[rt] = (short)mem_data_r.read().range(15,0);
					else
						r[rt] = (short)mem_data_r.read().range(31,16);
				else
					if (big_endian)
						r[rt] = (short)mem_data_r.read().range(31,16);
					else
						r[rt] = (short)mem_data_r.read().range(15,0);
			break;

			case 0x22:/*LWL*/  rt=rt; //fixme fall through
			case 0x23:/*LW*/
				//assert((ptr & 3) == 0);
				mem_address.write(ptr & word_addr);	// Address the memory with word address.
				wait(1);
				
				load_inst_kernel=(page != 0? load_inst_kernel : load_inst_kernel + 1 );
				load_inst_tasks=(page != 0? load_inst_tasks + 1  : load_inst_tasks );

				// Verifies the mem_pause signal at the first execution cycle
				if ( mem_pause.read() ) {
					mem_address.write(pc_last);	// Keep the last memory address before mem_pause = '1'
					wait(1);
					while (mem_pause.read())	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(ptr & word_addr);// Address the memory with word address.
					wait(1);
				}

				prefetch = true;
				prefetched_opcode = mem_data_r.read();
				mem_address.write(state->pc);
				wait(1);

				// Verifies the mem_pause signal at the second execution cycle
				if (mem_pause.read()) {
					mem_address.write(ptr & word_addr);// Keep the last memory address before mem_pause = '1'

					while (mem_pause.read())	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(state->pc);// Address the next instruction
					wait(1);
				}

				r[rt] = mem_data_r.read();
			break;

			case 0x24:/*LBU*/
				mem_address.write(ptr & word_addr);	// Address the memory with word address.
				wait(1);
				load_inst_kernel=(page != 0? load_inst_kernel : load_inst_kernel + 1 );
				load_inst_tasks=(page != 0? load_inst_tasks + 1  : load_inst_tasks );

				// Verifies the mem_pause signal at the first execution cycle
				if ( mem_pause.read() ) {
					mem_address.write(pc_last);	// Keep the last memory address before mem_pause = '1'

					while (mem_pause.read())	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(ptr & word_addr);// Address the memory with word address.
					wait(1);
				}

				prefetch = true;
				prefetched_opcode = mem_data_r.read();
				mem_address.write(state->pc);
				wait(1);

				// Verifies the mem_pause signal at the second execution cycle
				if (mem_pause.read()) {
					mem_address.write(ptr & word_addr);// Keep the last memory address before mem_pause = '1'

					while (mem_pause.read())	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(state->pc);// Address the next instruction
					wait(1);
				}

				if ((ptr & 3) == 3)
					if (big_endian)
							r[rt] = (unsigned char)mem_data_r.read().range(7,0);
						else
							r[rt] = (unsigned char)mem_data_r.read().range(31,24);

					else if ((ptr & 2) == 2)
						if (big_endian)
							r[rt] = (unsigned char)mem_data_r.read().range(15,8);
						else
							r[rt] = (unsigned char)mem_data_r.read().range(23,16);

					else if ((ptr & 1) == 1)
						if (big_endian)
							r[rt] = (unsigned char)mem_data_r.read().range(23,16);
						else
							r[rt] = (unsigned char)mem_data_r.read().range(15,8);

					else
						if (big_endian)
							r[rt] = (unsigned char)mem_data_r.read().range(31,24);
						else
							r[rt] = (unsigned char)mem_data_r.read().range(7,0);
			break;

			case 0x25:/*LHU*/
				//assert((ptr & 1) == 0);
				mem_address.write(ptr & word_addr);	// Address the memory with word address.
				wait(1);
				load_inst_kernel=(page != 0? load_inst_kernel : load_inst_kernel + 1 );
				load_inst_tasks=(page != 0? load_inst_tasks + 1  : load_inst_tasks );

				// Verifies the mem_pause signal at the first execution cycle
				if ( mem_pause.read() ) {
					mem_address.write(pc_last);	// Keep the last memory address before mem_pause = '1'
					wait(1);
					while (mem_pause.read())	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(ptr & word_addr);// Address the memory with word address.
					wait(1);
				}

				prefetch = true;
				prefetched_opcode = mem_data_r.read();
				mem_address.write(state->pc);
				wait(1);

				// Verifies the mem_pause signal at the second execution cycle
				if (mem_pause.read()) {
					mem_address.write(ptr & word_addr);// Keep the last memory address before mem_pause = '1'

					while (mem_pause.read())	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(state->pc);// Address the next instruction
					wait(1);
				}

				if ((ptr & 2) == 2)
					if (big_endian)
						r[rt] = (unsigned short)mem_data_r.read().range(15,0);
					else
						r[rt] = (unsigned short)mem_data_r.read().range(31,16);
				else
					if (big_endian)
						r[rt] = (unsigned short)mem_data_r.read().range(31,16);
					else
						r[rt] = ( unsigned short)mem_data_r.read().range(15,0);
			break;

			case 0x26:/*LWR*/  
				wait(1); 
				load_inst_kernel=(page != 0? load_inst_kernel : load_inst_kernel + 1 );
				load_inst_tasks=(page != 0? load_inst_tasks + 1  : load_inst_tasks );

			break; //fixme

			case 0x28:/*SB*/
				byte_write = r[rt] & 0x000000FF;	/* Retrieves the byte to be stored */
				mem_address.write(ptr & word_addr);	// Address the memory with word address.
				mem_data_w.write((byte_write<<24) | (byte_write<<16) | (byte_write<<8) | byte_write);

				if ((ptr & 3) == 3)
					if (big_endian)
						byte_en = 0x1;
					else
						byte_en = 0x8;

				else if ((ptr & 2) == 2)
					if (big_endian)
						byte_en = 0x2;
					else
						byte_en = 0x4;

				else if ((ptr & 1) == 1)
					if (big_endian)
						byte_en = 0x4;
					else
						byte_en = 0x2;

				else
					if (big_endian)
						byte_en = 0x8;
					else
						byte_en = 0x1;

				mem_byte_we.write(byte_en);

				wait(1);

				// Verifies the mem_pause signal at the first execution cycle
				if ( mem_pause ) {
					mem_address.write(pc_last);// Keep the last memory address before mem_pause = '1'
					mem_byte_we.write(0);// Disable write

					while (mem_pause)	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(ptr & word_addr);	// Address the memory with word address.
					mem_byte_we.write(byte_en);			// Enable write
					wait(1);
				}

				mem_byte_we.write(0x0);
				prefetch = true;
				prefetched_opcode = mem_data_r.read();
				mem_address.write(state->pc);
				wait(1);

				// Verifies the mem_pause signal at the second execution cycle
				if (mem_pause) {
					mem_address.write(ptr & word_addr);// Keep the last memory address before mem_pause = '1'

					while (mem_pause)	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(state->pc);// Address the next instruction
					wait(1);
				}
				
				load_inst_kernel=(page != 0? load_inst_kernel : load_inst_kernel + 1 );
				load_inst_tasks=(page != 0? load_inst_tasks + 1  : load_inst_tasks );
			break;

			case 0x29:/*SH*/
				byte_write = r[rt] & 0x0000FFFF;	/* Retrieves the half word to be stored */
				mem_address.write(ptr & word_addr);	// Address the memory with word address.
				mem_data_w.write((byte_write<<16) | byte_write);

				if ((ptr & 2) == 2)
					if (big_endian)
						byte_en = 0x3;
					else
						byte_en = 0xC;
				else
					if (big_endian)
						byte_en = 0xC;
					else
						byte_en = 0x3;

				mem_byte_we.write(byte_en);

				wait(1);

				// Verifies the mem_pause signal at the first execution cycle
				if ( mem_pause ) {
					mem_address.write(pc_last);// Keep the last memory address before mem_pause = '1'
					mem_byte_we.write(0);// Disable write

					while (mem_pause)	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(ptr & word_addr);	// Address the memory with word address.
					mem_byte_we.write(byte_en);			// Enable write
					wait(1);
				}

				mem_byte_we.write(0x0);
				prefetch = true;
				prefetched_opcode = mem_data_r.read();
				mem_address.write(state->pc);
				wait(1);

				// Verifies the mem_pause signal at the second execution cycle
				if (mem_pause) {
					mem_address.write(ptr & word_addr);// Keep the last memory address before mem_pause = '1'

					while (mem_pause)	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(state->pc);// Address the next instruction
					wait(1);
				}
				load_inst_kernel=(page != 0? load_inst_kernel : load_inst_kernel + 1 );
				load_inst_tasks=(page != 0? load_inst_tasks + 1  : load_inst_tasks );

			break;

			case 0x2a:/*SWL*/  rt=rt; //fixme fall through
			case 0x2b:/*SW*/
				//assert((ptr & 3) == 0);
				mem_address.write(ptr);
				mem_data_w.write(r[rt]);
				mem_byte_we.write(0xF);
				wait(1);

				// Verifies the mem_pause signal at the first execution cycle
				if ( mem_pause ) {
					mem_address.write(pc_last);// Keep the last memory address before mem_pause = '1'
					mem_byte_we.write(0);// Disable write

					while (mem_pause)	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(ptr & word_addr);	// Address the memory with word address.
					mem_byte_we.write(0xF);			// Enable write
					wait(1);
				}

				mem_byte_we.write(0x0);
				prefetch = true;
				prefetched_opcode = mem_data_r.read();
				mem_address.write(state->pc);
				wait(1);

				// Verifies the mem_pause signal at the second execution cycle
				if (mem_pause) {
					mem_address.write(ptr & word_addr);// Keep the last memory address before mem_pause = '1'
					while (mem_pause)	// Stalls the CPU while mem_pause = '1'
						wait(1);

					mem_address.write(state->pc);// Address the next instruction
					wait(1);
				}
				load_inst_kernel=(page != 0? load_inst_kernel : load_inst_kernel + 1 );
				load_inst_tasks=(page != 0? load_inst_tasks + 1  : load_inst_tasks );

			break;

			case 0x2e:/*SWR*/  
				wait(1); 
				other_inst_kernel=(page != 0? other_inst_kernel : other_inst_kernel + 1 );
				other_inst_tasks=(page != 0? other_inst_tasks + 1  : other_inst_tasks );

			break; //fixme
			case 0x2f:/*CACHE*/
				wait(1); 
				other_inst_kernel=(page != 0? other_inst_kernel : other_inst_kernel + 1 );
				other_inst_tasks=(page != 0? other_inst_tasks + 1  : other_inst_tasks );

			break;

			case 0x30:/*LL*/
				//assert((ptr & 3) == 0);
				mem_address.write(ptr);
				wait(1);

				prefetch = true;
				prefetched_opcode = mem_data_r.read();
				mem_address.write(state->pc);
				wait(1);

				r[rt] = mem_data_r.read();
				
				load_inst_kernel=(page != 0? load_inst_kernel : load_inst_kernel + 1 );
				load_inst_tasks=(page != 0? load_inst_tasks + 1  : load_inst_tasks );

			break;

	//      case 0x31:/*LWC1*/ break;
	//      case 0x32:/*LWC2*/ break;
	//      case 0x33:/*LWC3*/ break;
	//      case 0x35:/*LDC1*/ break;
	//      case 0x36:/*LDC2*/ break;
	//      case 0x37:/*LDC3*/ break;
	//      case 0x38:/*SC*/     *(int*)ptr=r[rt]; r[rt]=1; break;

			case 0x38:/*SC*/
				//assert((ptr & 3) == 0);
				mem_address.write(ptr);
				mem_data_w.write(r[rt]);
				mem_byte_we.write(0xF);
				wait(1);

				mem_byte_we.write(0x0);
				prefetch = true;
				prefetched_opcode = mem_data_r.read();
				mem_address.write(state->pc);
				wait(1);

				r[rt] = 1;
				
				load_inst_kernel=(page != 0? load_inst_kernel : load_inst_kernel + 1 );
				load_inst_tasks=(page != 0? load_inst_tasks + 1  : load_inst_tasks );

			break;
	//
			case 0x39:/*SWC1*/ 
				wait(1); 
				other_inst_kernel=(page != 0? other_inst_kernel : other_inst_kernel + 1 );
				other_inst_tasks=(page != 0? other_inst_tasks + 1  : other_inst_tasks );

			break;
	//      case 0x3a:/*SWC2*/ break;
	//      case 0x3b:/*SWC3*/ break;
	//      case 0x3d:/*SDC1*/ break;
	//      case 0x3e:/*SDC2*/ break;
	//      case 0x3f:/*SDC3*/ break;
			default:
				printf("\nPE: %x ERROR2 address=%lu opcode=%u\n",(int)address_router, state->pc,opcode);
				return;									
		}	// switch()

		global_inst_kernel= logical_inst_kernel + branch_inst_kernel + jump_inst_kernel + move_inst_kernel + other_inst_kernel + arith_inst_kernel + load_inst_kernel + shift_inst_kernel + nop_inst_kernel + mult_div_inst_kernel;
		global_inst_tasks= logical_inst_tasks + branch_inst_tasks + jump_inst_tasks + move_inst_tasks + other_inst_tasks + arith_inst_tasks + load_inst_tasks + shift_inst_tasks + nop_inst_tasks + mult_div_inst_tasks;
		

		logical_inst					= logical_inst_kernel + logical_inst_tasks;
		branch_inst					= branch_inst_kernel + branch_inst_tasks;
		jump_inst					= jump_inst_kernel + jump_inst_tasks;
		move_inst					= move_inst_kernel + move_inst_tasks;
		other_inst					= other_inst_kernel + other_inst_tasks;
		arith_inst					= arith_inst_kernel + arith_inst_tasks;
		load_inst					= load_inst_kernel + load_inst_tasks;
		shift_inst					= shift_inst_kernel + shift_inst_tasks;			
		nop_inst					= nop_inst_kernel + nop_inst_tasks;			
		mult_div_inst					= mult_div_inst_kernel + mult_div_inst_tasks;
		global_inst = global_inst_kernel + global_inst_tasks;
	}

}


void mlite_cpu::mult_big(unsigned int a, unsigned int b) {

	sc_uint <64> ahi, alo, bhi, blo;
	sc_uint <64> c0, c1, c2;
	sc_uint <64> c1_a, c1_b;

	//printf("mult_big(0x%x, 0x%x)\n", a, b);
	ahi = a >> 16;
	alo = a & 0xffff;
	bhi = b >> 16;
	blo = b & 0xffff;

	c0 = alo * blo;
	c1_a = ahi * blo;
	c1_b = alo * bhi;
	c2 = ahi * bhi;

	c2 += (c1_a >> 16) + (c1_b >> 16);
	c1 = (c1_a & 0xffff) + (c1_b & 0xffff) + (c0 >> 16);
	c2 += (c1 >> 16);
	c0 = (c1 << 16) + (c0 & 0xffff);
	//printf("answer=0x%x 0x%x\n", c2, c0);
	state->hi = c2;
	state->lo = c0;
}

void mlite_cpu::mult_big_signed(int a, int b) {

	sc_uint <64> ahi, alo, bhi, blo;
	sc_uint <64> c0, c1, c2;
	sc_uint <64> c1_a, c1_b;

	//printf("mult_big_signed(0x%x, 0x%x)\n", a, b);
	ahi = a >> 16;
	alo = a & 0xffff;
	bhi = b >> 16;
	blo = b & 0xffff;

	c0 = alo * blo;
	c1_a = ahi * blo;
	c1_b = alo * bhi;
	c2 = ahi * bhi;

	c2 += (c1_a >> 16) + (c1_b >> 16);
	c1 = (c1_a & 0xffff) + (c1_b & 0xffff) + (c0 >> 16);
	c2 += (c1 >> 16);
	c0 = (c1 << 16) + (c0 & 0xffff);
	//printf("answer=0x%x 0x%x\n", c2, c0);
	state->hi = c2;
	state->lo = c0;
}

