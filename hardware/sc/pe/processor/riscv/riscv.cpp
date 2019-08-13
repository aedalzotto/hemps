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

		(this->*execute)();
		x[0] = 0;
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
	wait(17);
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
		pc.next();
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
						pc.next();
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
	register_t ret = mem_data_r.read();
	wait(1);
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
		sepc.write(pc.read());					// Previous PC
		pc.write(stvec.BASE());					// Synchronous exceptions are always DIRECT
	} else {	// Handle in M-Mode
		mcause.interrupt() = 0;
		mcause.exception_code() = code;
		mtval.write(0);
		mstatus.MPP() = (uint32_t)priv.get();	// Previous privilege
		priv.set(Privilege::Level::MACHINE);	// New privilege
		mstatus.MPIE() = mstatus.MIE();			// Previous interrupt-enable of target mode
		mstatus.MIE() = 0;						// Disable interrupt-enable of target mode
		mepc.write(pc.read());					// Previous PC
		pc.write(mtvec.BASE());					// Synchronous exceptions are always DIRECT
	}
}

bool RiscV::decode()
{
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
			execute = &RiscV::jal;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::OPCODES::BRANCH:
		branch();
		break;
	case Instructions::OPCODES::LOAD:
		load();
		break;
	case Instructions::OPCODES::STORE:
		store();
		break;
	case Instructions::OPCODES::MISC_MEM:
		misc_mem();
		break;
	case Instructions::OPCODES::SYSTEM:
		system();
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
		switch(instr.imm_11_5()){
		case Instructions::IMM_11_5::SLLI:
			execute = &RiscV::slli;
			break;
		default:		
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::FUNCT3::SRLI_SRAI:
		switch(instr.imm_11_5()){
		case Instructions::IMM_11_5::SRAI:
			execute = &RiscV::srai;
			break;
		case Instructions::IMM_11_5::SRLI:
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
	// Decodes the funct7 and the funct3
	switch(instr.funct3()){
	case Instructions::FUNCT3::ADD_SUB:
		switch(instr.funct7()){
		case Instructions::FUNCT7::ADD:
			execute = &RiscV::add;
			break;
		case Instructions::FUNCT7::SUB:
			execute = &RiscV::sub;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::FUNCT3::SLL:
		switch(instr.funct7()){
		case Instructions::FUNCT7::SLL:
			execute = &RiscV::sll;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::FUNCT3::SLT:
		switch(instr.funct7()){
		case Instructions::FUNCT7::SLT:
			execute = &RiscV::slt;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::FUNCT3::SLTU:
		switch(instr.funct7()){
		case Instructions::FUNCT7::SLTU:
			execute = &RiscV::sltu;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::FUNCT3::XOR:
		switch(instr.funct7()){
		case Instructions::FUNCT7::XOR:
			execute = &RiscV::xor;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::FUNCT3::SRL_SRA:
		switch(instr.funct7()){
		case Instructions::FUNCT7::SRL:
			execute = &RiscV::srl;
			break;
		case Instructions::FUNCT7::SRA:
			execute = &RiscV::sra;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::FUNCT3::OR:
		switch(instr.funct7()){
		case Instructions::FUNCT7::OR:
			execute = &RiscV::or;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	case Instructions::FUNCT3::AND:
		switch(instr.funct7()){
		case Instructions::FUNCT7::AND:
			execute = &RiscV::and;
			break;
		default:
			handle_exceptions(Exceptions::CODE::ILLEGAL_INSTRUCTION);
			return true;
		}
		break;
	}
	return false;
}