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
	sensitive << clock.pos();// << mem_pause.pos();
	//sensitive << mem_pause.neg();
}

void RiscV::cpu()
{
	reset();

	while(true) {
		// @todo Save pc on mem_pause ??
		// @todo Inform externally of current page ??

		x[0].write(0);

		if(reset_in.read()){
			reset();
			continue;
		}

		mip.MEI() = intr_in.read();
		if(handle_interrupts())	// If interrupt is handled, continues interrupt PC
			continue;

		if(fetch())	// If exception occurred, continues to exception PC
			continue;

		if(decode()) // If exception occurred, continues to exception PC
			continue;

		if((this->*execute)())
			continue;

		pc.next();
	}

}

void RiscV::reset()
{
	priv.set(Privilege::Level::MACHINE);
	mstatus.MIE() = 0;
	mstatus.MPRV() = 0;
	misa.write((ISA::Ext::M | ISA::Ext::S | ISA::Ext::U));
	pc.write(vectors::RESET);
	mcause.write(0);

	// mem_byte_we.write(0x0);
	wait(Timings::RESET);
}

bool RiscV::handle_interrupts()
{
	// Machine-level interrupt. Can only be masked by M-Mode
	if((mip.read() & mie.read() & ~mideleg.read()) && 
					(priv.get() != Privilege::Level::MACHINE || mstatus.MIE())){
		if(mip.MEI() && mie.MEI() && !mideleg.MEI()) // Machine External Interrupt
			mcause.exception_code() = Interrupts::CODE::MEI;
		else if(mip.MSI() && mie.MSI() && !mideleg.MSI()) // Machine Software Interrupt
			mcause.exception_code() = Interrupts::CODE::MSI;
		else if(mip.MTI() && mie.MTI() && !mideleg.MTI()) // Machine Timer Interrupt
			mcause.exception_code() = Interrupts::CODE::MTI;
		else if(mip.SEI() && mie.SEI() && !mideleg.SEI()) // Supervisor External Interrupt
			mcause.exception_code() = Interrupts::CODE::SEI;
		else if(mip.SSI() && mie.SSI() && !mideleg.SSI()) // Supervisor Software Interrupt
			mcause.exception_code() = Interrupts::CODE::SSI;
		else if(mip.STI() && mie.STI() && !mideleg.STI()) // Supervisor Timer Interrupt
			mcause.exception_code() = Interrupts::CODE::STI;
		
		mcause.interrupt() = 1;
		mtval.write(0);
		mstatus.MPP() = (uint32_t)priv.get(); // Previous privilege
		priv.set(Privilege::Level::MACHINE);  // New privilege
		mstatus.MPIE() = mstatus.MIE();		  // Previous interrupt-enable of target mode
		mstatus.MIE() = 0;					  // Disable interrupt-enable of target mode
		mepc.write(pc.read());				  // Previous PC
		if(mtvec.MODE() == (uint32_t)Mtvec::Mode::VECTORED){ // New pc
			pc.write(mtvec.BASE() + 4 * mcause.exception_code());
		} else { // Direct
			pc.write(mtvec.BASE());
		}
		return true;	// Interrupt taken
	} else if((mip.read() & mie.read() & mideleg.read()) && // Supervisor-level interrupt
				(priv.get() == Privilege::Level::USER || 	// U-Mode can't mask
				(priv.get() == Privilege::Level::SUPERVISOR && mstatus.SIE()))){ // S-Mode interrupt enabled
		if(mip.MEI() && mie.MEI() && !mideleg.MEI()) // Machine External Interrupt
			scause.exception_code() = Interrupts::CODE::MEI;
		else if(mip.MSI() && mie.MSI() && !mideleg.MSI()) // Machine Software Interrupt
			scause.exception_code() = Interrupts::CODE::MSI;
		else if(mip.MTI() && mie.MTI() && !mideleg.MTI()) // Machine Timer Interrupt
			scause.exception_code() = Interrupts::CODE::MTI;
		else if(mip.SEI() && mie.SEI() && !mideleg.SEI()) // Supervisor External Interrupt
			scause.exception_code() = Interrupts::CODE::SEI;
		else if(mip.SSI() && mie.SSI() && !mideleg.SSI()) // Supervisor Software Interrupt
			scause.exception_code() = Interrupts::CODE::SSI;
		else if(mip.STI() && mie.STI() && !mideleg.STI()) // Supervisor Timer Interrupt
			scause.exception_code() = Interrupts::CODE::STI;
		
		scause.interrupt() = 1;
		stval.write(0);
		mstatus.SPP() = (uint32_t)priv.get();	// Previous privilege
		priv.set(Privilege::Level::SUPERVISOR);	// New privilege
		mstatus.SPIE() = mstatus.SIE();			// Previous interrupt-enable of target mode
		mstatus.SIE() = 0;						// Disable interrupt-enable of target mode
		sepc.write(pc.read());					// Previous PC
		if(stvec.MODE() == (uint32_t)Mtvec::Mode::VECTORED){ // New pc
			pc.write(stvec.BASE() + 4 * scause.exception_code());
		} else { // Direct
			pc.write(stvec.BASE());
		}
		return true;	// Interrupt taken
	} // User-level interrupts not implemented
	return false;	// Interrupt not taken
}

bool RiscV::fetch()
{
	if(priv.get() == Privilege::Level::MACHINE || satp.MODE() == Satp::MODES::BARE){
		instr.write(mem_read(pc.read()));
		return false;
	} else { // Sv32
		Sv32::VirtualAddress va(pc);
		Sv32::PhysicalAddress a(satp.PPN() * Sv32::PAGESIZE);
		for(int i = Sv32::LEVELS - 1; i >= 0; i--){
			Sv32::PhysicalAddress pte_addr(a.read() + va.VPN(i)*Sv32::PTESIZE);
			Sv32::PageTableEntry pte;
			pte.write(mem_read(pte_addr.read()));
			if(!pte.V() || (!pte.R() && pte.W())){
				// Not valid or Write-Only
				handle_exceptions(Exceptions::CODE::INSTRUCTION_PAGE_FAULT);
				return true;
			} else if(pte.R() || pte.X()){
				// Leaf PTE
				if(pte.X() && 
					((priv.get() == Privilege::Level::USER && pte.U()) || 
					 (priv.get() == Privilege::Level::SUPERVISOR && !pte.U()))){
					// Memory execute allowed
					if(i && pte.PPN(0)){
						// Misaligned superpage
						handle_exceptions(Exceptions::CODE::INSTRUCTION_PAGE_FAULT);
						return true;
					} else {
						// Efectively access PTE
						pte.A() = 1;
						Sv32::PhysicalAddress pa(0);
						pa.page_offset() = va.page_offset();
						if(i){
							// Superpage translation
							pa.PPN(0) = va.VPN(0);
						}
						pa.PPN(Sv32::LEVELS - 1) = pte.PPN(Sv32::LEVELS - 1);
						instr.write(mem_read(pa.PPN()*Sv32::PAGESIZE + pa.page_offset()));
						return false;
					}
				} else {
					// Invalid access type
					handle_exceptions(Exceptions::CODE::INSTRUCTION_PAGE_FAULT);
					return true;
				}
			} else {
				// Pointer to next-level PTE
				a.write(pte.PPN() * Sv32::PAGESIZE);
				continue;
			}
		}
		handle_exceptions(Exceptions::CODE::INSTRUCTION_PAGE_FAULT);
		return true;
	}
}

register_t RiscV::mem_read(sc_uint<34> address)
{
	mem_address.write(address);
	wait(Timings::MEM_READ);
	register_t ret = mem_data_r.read();
	return ret;
}

void RiscV::handle_exceptions(Exceptions::CODE code)
{
	if(priv.get() != Privilege::Level::MACHINE && (medeleg.read() & (1 << code))){ // Handle in S-Mode
		scause.interrupt() = 0;
		scause.exception_code() = code;
		stval.write(0);
		mstatus.SPP() = (uint32_t)priv.get();	// Previous privilege
		priv.set(Privilege::Level::SUPERVISOR);	// New privilege
		mstatus.SPIE() = mstatus.SIE();			// Previous interrupt-enable of target mode
		mstatus.SIE() = 0;						// Disable interrupt-enable of target mode
		sepc.write(pc.read()+4);				// Previous PC
		pc.write(stvec.BASE());					// Synchronous exceptions are always DIRECT
	} else {	// Handle in M-Mode
		mcause.interrupt() = 0;
		mcause.exception_code() = code;
		mtval.write(0);
		mstatus.MPP() = (uint32_t)priv.get();	// Previous privilege
		priv.set(Privilege::Level::MACHINE);	// New privilege
		mstatus.MPIE() = mstatus.MIE();			// Previous interrupt-enable of target mode
		mstatus.MIE() = 0;						// Disable interrupt-enable of target mode
		mepc.write(pc.read()+4);				// Previous PC
		pc.write(mtvec.BASE());					// Synchronous exceptions are always DIRECT
	}
}

bool RiscV::decode()
{
	wait(Timings::DECODE);
	// First level of decoding. Decode the opcode
	switch(instr.opcode()){
	case Instructions::OPCODES::OP_IMM:
		return decode_op_imm();
		break;
	case Instructions::OPCODES::LUI:
		execute = &RiscV::lui;
		break;
	case Instructions::OPCODES::AUIPC:
		execute = &RiscV::auipc;
		break;
	case Instructions::OPCODES::OP:
		return decode_op();
		break;
	case Instructions::OPCODES::JAL:
		execute = &RiscV::jal;
		break;
	case Instructions::OPCODES::JALR:
		switch(instr.funct3()){
		case Instructions::FUNCT3::JALR:
			execute = &RiscV::jalr;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::OPCODES::BRANCH:
		return decode_branch();
		break;
	case Instructions::OPCODES::LOAD:
		return decode_load();
		break;
	case Instructions::OPCODES::STORE:
		return decode_store();
		break;
	case Instructions::OPCODES::MISC_MEM:
		switch(instr.funct3()){
		case Instructions::FUNCT3::FENCE:
			execute = &RiscV::fence;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::OPCODES::SYSTEM:
		return decode_system();
		break;
	}
	return false;
}

bool RiscV::decode_op_imm()
{
	// Decodes the imm[11:5] and the funct3
	switch(instr.funct3()){
	case Instructions::FUNCT3::ADDI:
		execute = &RiscV::addi;
		break;
	case Instructions::FUNCT3::SLTI:
		execute = &RiscV::slti;
		break;
	case Instructions::FUNCT3::SLTIU:
		execute = &RiscV::sltiu;
		break;
	case Instructions::FUNCT3::XORI:
		execute = &RiscV::xori;
		break;
	case Instructions::FUNCT3::ORI:
		execute = &RiscV::ori;
		break;
	case Instructions::FUNCT3::ANDI:
		execute = &RiscV::andi;
		break;
	case Instructions::FUNCT3::SLLI:
		switch(instr.funct7()){
		case Instructions::FUNCT7::SLLI:
			execute = &RiscV::slli;
			break;
		default:		
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::FUNCT3::SRLI_SRAI:
		switch(instr.funct7()){
		case Instructions::FUNCT7::SRAI:
			execute = &RiscV::srai;
			break;
		case Instructions::FUNCT7::SRLI:
			execute = &RiscV::srli;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	default:
		handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
		return true;
	}
	return false;
}

bool RiscV::decode_op()
{
	// Decodes funct7 first and then funct3
	switch(instr.funct7()){
	case Instructions::FUNCT7::SUB_SRA:
		switch(instr.funct3()){
		case Instructions::FUNCT3::SUB:
			execute = &RiscV::sub;
			break;
		case Instructions::FUNCT3::SRA:
			execute = &RiscV::sra;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::FUNCT7::MULDIV:
		switch(instr.funct3()){
		case Instructions::FUNCT3::MUL:
			execute = &RiscV::mul;
			break;
		case Instructions::FUNCT3::MULH:
			execute = &RiscV::mulh;
			break;
		case Instructions::FUNCT3::MULHSU:
			execute = &RiscV::mulhsu;
			break;
		case Instructions::FUNCT3::MULHU:
			execute = &RiscV::mulhu;
			break;
		case Instructions::FUNCT3::DIV:
			execute = &RiscV::div;
			break;
		case Instructions::FUNCT3::DIVU:
			execute = &RiscV::divu;
			break;
		case Instructions::FUNCT3::REM:
			execute = &RiscV::rem;
			break;
		case Instructions::FUNCT3::REMU:
			execute = &RiscV::remu;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	//case Instructions::FUNCT7::ADD_SLT_SLTU:
	//case Instructions::FUNCT7::AND_OR_XOR:
	//case Instructions::FUNCT7::SLL_SRL:
	case 0:
		switch(instr.funct3()){
		case Instructions::FUNCT3::ADD:
			execute = &RiscV::add;
			break;
		case Instructions::FUNCT3::SLL:
			execute = &RiscV::sll;
			break;
		case Instructions::FUNCT3::SLT:
			execute = &RiscV::slt;
			break;
		case Instructions::FUNCT3::SLTU:
			execute = &RiscV::sltu;
			break;
		case Instructions::FUNCT3::XOR:
			execute = &RiscV::_xor;
			break;
		case Instructions::FUNCT3::SRL:
			execute = &RiscV::srl;
			break;
		case Instructions::FUNCT3::OR:
			execute = &RiscV::_or;
			break;
		case Instructions::FUNCT3::AND:
			execute = &RiscV::_and;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	default:
		handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
		return true;
	}
	return false;
}

bool RiscV::decode_branch()
{
	switch(instr.funct3()){
	case Instructions::FUNCT3::BEQ:
		execute = &RiscV::beq;
		break;
	case Instructions::FUNCT3::BNE:
		execute = &RiscV::bne;
		break;
	case Instructions::FUNCT3::BLT:
		execute = &RiscV::blt;
		break;
	case Instructions::FUNCT3::BGE:
		execute = &RiscV::bge;
		break;
	case Instructions::FUNCT3::BLTU:
		execute = &RiscV::bltu;
		break;
	case Instructions::FUNCT3::BGEU:
		execute = &RiscV::bgeu;
		break;
	default:
		handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
		return true;
	}
	return false;
}

bool RiscV::decode_load()
{
	switch(instr.funct3()){
	case Instructions::FUNCT3::LB:
		execute = &RiscV::lb;
		break;
	case Instructions::FUNCT3::LH:
		execute = &RiscV::lh;
		break;
	case Instructions::FUNCT3::LW:
		execute = &RiscV::lw;
		break;
	case Instructions::FUNCT3::LBU:
		execute = &RiscV::lbu;
		break;
	case Instructions::FUNCT3::LHU:
		execute = &RiscV::lhu;
		break;
	default:
		handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
		return true;
	}
	return false;
}

bool RiscV::decode_store()
{
	switch(instr.funct3()){
	case Instructions::FUNCT3::SB:
		execute = &RiscV::sb;
		break;
	case Instructions::FUNCT3::SH:
		execute = &RiscV::sh;
		break;
	case Instructions::FUNCT3::SW:
		execute = &RiscV::sw;
		break;
	default:
		handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
		return true;
	}
	return false;
}

bool RiscV::decode_system()
{
	switch(instr.funct3()){
	case Instructions::FUNCT3::PRIV:
		switch(instr.funct7()){
		case Instructions::FUNCT7::ECALL_EBREAK:
			if(!(instr.rs1() || instr.rd())){
				switch(instr.rs2()){
				case Instructions::RS2::ECALL:
					execute = &RiscV::ecall;
					break;
				case Instructions::RS2::EBREAK:
					execute = &RiscV::ebreak;
					break;
				default:
					handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
					return true;
				}
			} else {
				handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
				return true;
			}
			break;
		case Instructions::FUNCT7::SRET_WFI:
			if(!(instr.rs1() || instr.rd())){
				switch(instr.rs2()){
				case Instructions::RS2::RET:
					execute = &RiscV::sret;
					break;
				case Instructions::RS2::WFI:
					execute = &RiscV::wfi;
					break;
				default:
					handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
					return true;
				}
			} else {
				handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
				return true;
			}
			break;
		case Instructions::FUNCT7::MRET:
			if(!(instr.rs1() || instr.rd()) && instr.rs2() == Instructions::RS2::RET){
				execute = &RiscV::mret;
			} else {
				handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
				return true;
			}
			break;
		case Instructions::FUNCT7::SFENCE_VMA:
			if(!instr.rd()){
				execute = &RiscV::sfence_vma;
			} else {
				handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
				return true;
			}
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::FUNCT3::CSRRW:
		execute = &RiscV::csrrw;
		break;
	case Instructions::FUNCT3::CSRRS:
		execute = &RiscV::csrrs;
		break;
	case Instructions::FUNCT3::CSRRC:
		execute = &RiscV::csrrc;
		break;
	case Instructions::FUNCT3::CSRRWI:
		execute = &RiscV::csrrwi;
		break;
	case Instructions::FUNCT3::CSRRSI:
		execute = &RiscV::csrrsi;
		break;
	case Instructions::FUNCT3::CSRRCI:
		execute = &RiscV::csrrci;
		break;
	default:
		handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
		return true;
	}
	return false;
}

bool RiscV::lui()
{
	wait(Timings::LOGICAL);
	x[instr.rd()].range(31,12) = instr.imm_31_12();
	x[instr.rd()].range(11,0) = 0;
	return false;
}

bool RiscV::auipc()
{
	wait(Timings::LOGICAL);
	Register r;
	r.range(31,12) = instr.imm_31_12();
	r.range(11,0) = 0;
	r.write(r.read() + pc.read());
	x[instr.rd()] = r;
	return false;
}

bool RiscV::jal()
{
	wait(Timings::LOGICAL);
	// Save PC ("Link")
	x[instr.rd()].write(pc.read()+4);

	// Sign-extend offset
	Register r;
	r.range(31,20) = (int)instr.imm_20() * -1;
	
	r.range(19, 12) = instr.imm_19_12();
	r.bit(11) = instr.imm_11_J();
	r.range(10, 1) = instr.imm_10_1();
	r.bit(0) = 0;
	
	r.write(r.read() + pc.read());

	if(r.read() % 4)
		handle_exceptions(Exceptions::CODE::INSTRUCTION_ADDRESS_MISALIGNED);
	else
		pc.write(r.read());
	

	return true;
}

bool RiscV::jalr()
{
	wait(Timings::LOGICAL);
	// Save PC ("Link")
	x[instr.rd()].write(pc.read()+4);

	// Sign-extend offset
	Register r;
	r.range(31,12) = ((int)instr.imm_11_0() >> 11) * -1;
	
	r.range(11, 0) = instr.imm_11_0();
	r.write(r.read() + pc.read());
	r.bit(0) = 0;

	r.write(r.read() + pc.read());

	if(r.read() % 4)
		handle_exceptions(Exceptions::CODE::INSTRUCTION_ADDRESS_MISALIGNED);
	else
		pc.write(r.read());

	return true;
}

bool RiscV::beq()
{
	wait(Timings::LOGICAL);
	if(x[instr.rs1()].read() == x[instr.rs2()].read()){ // Taken
		// Sign-extend offset
		Register r;
		r.range(31,12) = (int)instr.imm_12() * -1;
		r.bit(11) = instr.imm_11_B();
		r.range(10, 5) = instr.imm_10_5();
		r.range(4, 1) = instr.imm_4_1();
		r.bit(0) = 0;
		r.write(r.read() + pc.read());

		if(r.read() % 4)
			handle_exceptions(Exceptions::CODE::INSTRUCTION_ADDRESS_MISALIGNED);
		else
			pc.write(r.read());
		return true;
	} else { // Not taken
		return false;
	}
}

bool RiscV::bne()
{
	wait(Timings::LOGICAL);
	if(x[instr.rs1()].read() != x[instr.rs2()].read()){ // Taken
		// Sign-extend offset
		Register r;
		r.range(31,12) = (int)instr.imm_12() * -1;
		r.bit(11) = instr.imm_11_B();
		r.range(10, 5) = instr.imm_10_5();
		r.range(4, 1) = instr.imm_4_1();
		r.bit(0) = 0;
		r.write(r.read() + pc.read());

		if(r.read() % 4)
			handle_exceptions(Exceptions::CODE::INSTRUCTION_ADDRESS_MISALIGNED);
		else
			pc.write(r.read());
		return true;
	} else { // Not taken
		return false;
	}
}

bool RiscV::blt()
{
	wait(Timings::LOGICAL);
	if((int)x[instr.rs1()].read() < (int)x[instr.rs2()].read()){ // Taken
		// Sign-extend offset
		Register r;
		r.range(31,12) = (int)instr.imm_12() * -1;
		r.bit(11) = instr.imm_11_B();
		r.range(10, 5) = instr.imm_10_5();
		r.range(4, 1) = instr.imm_4_1();
		r.bit(0) = 0;
		r.write(r.read() + pc.read());

		if(r.read() % 4)
			handle_exceptions(Exceptions::CODE::INSTRUCTION_ADDRESS_MISALIGNED);
		else
			pc.write(r.read());
		return true;
	} else { // Not taken
		return false;
	}
}

bool RiscV::bge()
{
	wait(Timings::LOGICAL);
	if((int)x[instr.rs1()].read() >= (int)x[instr.rs2()].read()){ // Taken
		// Sign-extend offset
		Register r;
		r.range(31,12) = (int)instr.imm_12() * -1;
		r.bit(11) = instr.imm_11_B();
		r.range(10, 5) = instr.imm_10_5();
		r.range(4, 1) = instr.imm_4_1();
		r.bit(0) = 0;
		r.write(r.read() + pc.read());

		if(r.read() % 4)
			handle_exceptions(Exceptions::CODE::INSTRUCTION_ADDRESS_MISALIGNED);
		else
			pc.write(r.read());
		return true;
	} else { // Not taken
		return false;
	}
}

bool RiscV::bltu()
{
	wait(Timings::LOGICAL);
	if((unsigned int)x[instr.rs1()].read() < (unsigned int)x[instr.rs2()].read()){ // Taken
		// Sign-extend offset
		Register r;
		r.range(31,12) = (int)instr.imm_12() * -1;
		r.bit(11) = instr.imm_11_B();
		r.range(10, 5) = instr.imm_10_5();
		r.range(4, 1) = instr.imm_4_1();
		r.bit(0) = 0;
		r.write(r.read() + pc.read());

		if(r.read() % 4)
			handle_exceptions(Exceptions::CODE::INSTRUCTION_ADDRESS_MISALIGNED);
		else
			pc.write(r.read());
		return true;
	} else { // Not taken
		return false;
	}
}

bool RiscV::bgeu()
{
	wait(Timings::LOGICAL);
	if((unsigned int)x[instr.rs1()].read() >= (unsigned int)x[instr.rs2()].read()){ // Taken
		// Sign-extend offset
		Register r;
		r.range(31,12) = (int)instr.imm_12() * -1;
		r.bit(11) = instr.imm_11_B();
		r.range(10, 5) = instr.imm_10_5();
		r.range(4, 1) = instr.imm_4_1();
		r.bit(0) = 0;
		r.write(r.read() + pc.read());

		if(r.read() % 4)
			handle_exceptions(Exceptions::CODE::INSTRUCTION_ADDRESS_MISALIGNED);
		else
			pc.write(r.read());
		return true;
	} else { // Not taken
		return false;
	}
}

bool RiscV::lb()
{
	
}

bool RiscV::lh()
{

}

bool RiscV::lw()
{
	wait(Timings::LOGICAL);
	// Sign-extend offset
	Register r;
	r.range(31,12) = ((int)instr.imm_11_0() >> 11) * -1;
	r.range(11, 0) = instr.imm_11_0();
	r.write(r.read() + x[instr.rs1()].read());

	// Load Word must be 32-bit aligned
	const uint32_t offset = r.read() & 0x00000003;
	if(offset){
		handle_exceptions(Exceptions::CODE::INSTRUCTION_ADDRESS_MISALIGNED);
		return true;
	}

	const uint32_t address = r.read() & 0xFFFFFFFC;
	mem_address.write(address);	// Address the memory with word address (4-byte aligned).
	wait(1);

	// Verifies the mem_pause signal at the first execution cycle
	if(mem_pause.read()){
		mem_address.write(pc.read());	// Keep the last memory address before mem_pause = '1'
		wait(1);
		while (mem_pause.read())	// Stalls the CPU while mem_pause = '1'
			wait(1);

		mem_address.write(address);// Address the memory with word address.
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

	x[instr.rs2()].write(mem_data_r.read());
}

bool RiscV::lbu()
{

}

bool RiscV::lhu()
{

}

bool RiscV::sb()
{

}

bool RiscV::sh()
{

}

bool RiscV::sw()
{

}

bool RiscV::addi()
{
	wait(Timings::LOGICAL);
	// Sign-extend immediate
	Register r;
	r.range(31,12) = ((int)instr.imm_11_0() >> 11) * -1;
	r.range(11, 0) = instr.imm_11_0();

	x[instr.rd()].write(x[instr.rs1()].read() + r.read());

	return false;
}

bool RiscV::slti()
{
	wait(Timings::LOGICAL);
	// Sign-extend immediate
	Register r;
	r.range(31,12) = ((int)instr.imm_11_0() >> 11) * -1;
	r.range(11, 0) = instr.imm_11_0();

	x[instr.rd()].write(((int)x[instr.rs1()].read() < (int)r.read()));

	return false;
}

bool RiscV::sltiu()
{
	wait(Timings::LOGICAL);
	// Sign-extend immediate
	Register r;
	r.range(31,12) = ((int)instr.imm_11_0() >> 11) * -1;
	r.range(11, 0) = instr.imm_11_0();

	x[instr.rd()].write(((unsigned int)x[instr.rs1()].read() < (unsigned int)r.read()));

	return false;
}

bool RiscV::xori()
{
	wait(Timings::LOGICAL);
	// Sign-extend immediate
	Register r;
	r.range(31,12) = ((int)instr.imm_11_0() >> 11) * -1;
	r.range(11, 0) = instr.imm_11_0();

	x[instr.rd()].write(x[instr.rs1()].read() ^ r.read());

	return false;
}

bool RiscV::ori()
{
	wait(Timings::LOGICAL);
	// Sign-extend immediate
	Register r;
	r.range(31,12) = ((int)instr.imm_11_0() >> 11) * -1;
	r.range(11, 0) = instr.imm_11_0();

	x[instr.rd()].write(x[instr.rs1()].read() | r.read());

	return false;
}

bool RiscV::andi()
{
	wait(Timings::LOGICAL);
	// Sign-extend immediate
	Register r;
	r.range(31,12) = ((int)instr.imm_11_0() >> 11) * -1;
	r.range(11, 0) = instr.imm_11_0();

	x[instr.rd()].write(x[instr.rs1()].read() & r.read());

	return false;
}

bool RiscV::slli()
{
	wait(Timings::LOGICAL);
	x[instr.rd()].write(x[instr.rs1()].read() << instr.imm_4_0());

	return false;
}

bool RiscV::srli()
{
	wait(Timings::LOGICAL);
	x[instr.rd()].write(x[instr.rs1()].read() >> instr.imm_4_0());

	return false;
}

bool RiscV::srai()
{
	wait(Timings::LOGICAL);

	Register r;
	r.write(x[instr.rs1()].read());

	// Sign-extend
	uint32_t sign = 0xFFFFFFFF * r.bit(31);
	r.write(r.read() >> instr.imm_4_0());
	r.range(31, 31 - instr.imm_4_0()) = sign;

	x[instr.rd()].write(r.read());

	return false;
}

bool RiscV::add()
{
	wait(Timings::LOGICAL);

	x[instr.rd()].write(x[instr.rs1()].read() + x[instr.rs2()].read());

	return false;
}

bool RiscV::sub()
{
	wait(Timings::LOGICAL);

	x[instr.rd()].write(x[instr.rs1()].read() - x[instr.rs2()].read());

	return false;
}

bool RiscV::sll()
{
	wait(Timings::LOGICAL);

	x[instr.rd()].write(x[instr.rs1()].read() << x[instr.rs2()].range(4, 0));

	return false;
}

bool RiscV::slt()
{
	wait(Timings::LOGICAL);

	x[instr.rd()].write(((int)x[instr.rs1()].read() < (int)x[instr.rs2()].read()));

	return false;
}

bool RiscV::sltu()
{
	wait(Timings::LOGICAL);

	x[instr.rd()].write(((unsigned int)x[instr.rs1()].read() < (unsigned int)x[instr.rs2()].read()));

	return false;
}

bool RiscV::_xor()
{
	wait(Timings::LOGICAL);

	x[instr.rd()].write(x[instr.rs1()].read() ^ x[instr.rs2()].read());

	return false;
}

bool RiscV::srl()
{
	wait(Timings::LOGICAL);

	x[instr.rd()].write(x[instr.rs1()].read() >> x[instr.rs2()].range(4,0));

	return false;
}

bool RiscV::sra()
{
	wait(Timings::LOGICAL);

	Register r;
	r.write(x[instr.rs1()].read());

	// Sign-extend
	uint32_t sign = 0xFFFFFFFF * r.bit(31);
	r.write(r.read() >> x[instr.rs2()].range(4, 0));
	r.range(31, 31 - x[instr.rs2()].range(4, 0)) = sign;

	x[instr.rd()].write(r.read());

	return false;
}

bool RiscV::_or()
{
	wait(Timings::LOGICAL);

	x[instr.rd()].write(x[instr.rs1()].read() | x[instr.rs2()].read());

	return false;
}

bool RiscV::_and()
{
	wait(Timings::LOGICAL);

	x[instr.rd()].write(x[instr.rs1()].read() & x[instr.rs2()].read());

	return false;
}

bool RiscV::fence()
{
	wait(Timings::LOGICAL);

	return false;
}

bool RiscV::ecall()
{
	wait(Timings::LOGICAL);

	switch(priv.get()){
	case Privilege::Level::MACHINE:
		handle_exceptions(Exceptions::CODE::ECALL_FROM_MMODE);
		break;
	case Privilege::Level::SUPERVISOR:
		handle_exceptions(Exceptions::CODE::ECALL_FROM_SMODE);
		break;
	case Privilege::Level::USER:
		handle_exceptions(Exceptions::CODE::ECALL_FROM_UMODE);
		break;
	}	

	return true;
}

bool RiscV::ebreak()
{
	wait(Timings::LOGICAL);

	return false;
}

bool RiscV::mul()
{
	wait(Timings::MUL);

	x[instr.rd()].write((uint32_t)x[instr.rs1()].read() * (uint32_t)x[instr.rs2()].read());

	return false;
}

bool RiscV::mulh()
{
	wait(Timings::MUL);

	uint64_t res = (int64_t)x[instr.rs1()].read() * (int64_t)x[instr.rs2()].read();
	uint32_t high = res >> 32;
	
	x[instr.rd()].write(high);

	return false;
}

bool RiscV::mulhsu()
{
	wait(Timings::MUL);

	uint64_t res = (int64_t)x[instr.rs1()].read() * (uint64_t)x[instr.rs2()].read();
	uint32_t high = res >> 32;
	
	x[instr.rd()].write(high);

	return false;
}

bool RiscV::mulhu()
{
	wait(Timings::MUL);

	uint64_t res = (uint64_t)x[instr.rs1()].read() * (uint64_t)x[instr.rs2()].read();
	uint32_t high = res >> 32;
	
	x[instr.rd()].write(high);

	return false;
}

bool RiscV::div()
{
	wait(Timings::DIV);

	if(!x[instr.rs1()].read()){ // 0 divided by anything is 0
		x[instr.rd()].write(0);
	} else if(!x[instr.rs2()].read()){ // Div by 0
		x[instr.rd()].write((uint32_t)-1); // Max out
	} else if(x[instr.rs1()].read() == 1 && (int32_t)x[instr.rs2()].read() == -1){ // Overflow
		x[instr.rd()].write(x[instr.rs1()].read()); // Writes the dividend
	} else {
		x[instr.rd()].write((int32_t)x[instr.rs1()].read() / (int32_t)x[instr.rs2()].read());
	}

	return false;
}

bool RiscV::divu()
{
	wait(Timings::DIV);

	if(!x[instr.rs1()].read()){ // 0 divided by anything is 0
		x[instr.rd()].write(0);
	} else if(!x[instr.rs2()].read()){ // Div by 0
		x[instr.rd()].write((uint32_t)-1); // Max out
	} else {
		x[instr.rd()].write((uint32_t)x[instr.rs1()].read() / (uint32_t)x[instr.rs2()].read());
	}

	return false;
}

bool RiscV::rem()
{
	wait(Timings::DIV);

	if(!x[instr.rs1()].read()){ // 0 divided by anything is 0
		x[instr.rd()].write(0);
	} else if(!x[instr.rs2()].read()){ // Div by 0
		x[instr.rd()].write(x[instr.rs1()].read()); // Writes the dividend
	} else if(x[instr.rs1()].read() == 1 && (int32_t)x[instr.rs2()].read() == -1){ // Overflow
		x[instr.rd()].write(0); // No remainder
	} else {
		x[instr.rd()].write((int32_t)x[instr.rs1()].read() % (int32_t)x[instr.rs2()].read());
	}

	return false;
}

bool RiscV::remu()
{
	wait(Timings::DIV);

	if(!x[instr.rs1()].read()){ // 0 divided by anything is 0
		x[instr.rd()].write(0);
	} else if(!x[instr.rs2()].read()){ // Div by 0
		x[instr.rd()].write(x[instr.rs1()].read()); // Writes the dividend
	} else {
		x[instr.rd()].write((uint32_t)x[instr.rs1()].read() % (uint32_t)x[instr.rs2()].read());
	}

	return false;
}

bool RiscV::csrrw()
{

}

bool RiscV::csrrs()
{

}

bool RiscV::csrrc()
{

}

bool RiscV::csrrwi()
{

}

bool RiscV::csrrsi()
{

}

bool RiscV::csrrci()
{
	
}

bool RiscV::sret()
{
	wait(Timings::LOGICAL);

	// Can only be called in M and S-Mode and if SRET trap is disabled
	if(priv.get() == Privilege::Level::USER || mstatus.TSR()){
		handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
	} else {
		// Manipulate the privilege stack
		mstatus.SIE() = mstatus.SPIE();
		priv.set((Privilege::Level)((uint32_t)mstatus.SPP()));
		mstatus.SPIE() = 1;
		mstatus.SPP() = (uint32_t)Privilege::Level::USER;

		// Return to exception pc
		pc.write(sepc.read());
	}

	return true;
}

bool RiscV::mret()
{
	wait(Timings::LOGICAL);

	// Can only be called in M-Mode
	if(priv.get() != Privilege::Level::MACHINE){
		handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
	} else {
		// Manipulate the privilege stack
		mstatus.MIE() = mstatus.MPIE();
		priv.set((Privilege::Level)((uint32_t)mstatus.MPP()));
		mstatus.MPIE() = 1;
		mstatus.MPP() = (uint32_t)Privilege::Level::USER;

		// Return to exception pc
		pc.write(mepc.read());
	}

	return true;
}

bool RiscV::wfi()
{
	wait(Timings::LOGICAL);

	// Not available in U-Mode or if timeout wait in S-Mode
	if(priv.get() == Privilege::Level::USER || (mstatus.TW() && priv.get() == Privilege::Level::SUPERVISOR)){
		handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
		return true;
	}
	
	// NOP

	return false;
}

bool RiscV::sfence_vma()
{
	wait(Timings::LOGICAL);

	if(priv.get() == Privilege::Level::USER){
		handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
		return true;
	}

	// NOP

	return false;
}

