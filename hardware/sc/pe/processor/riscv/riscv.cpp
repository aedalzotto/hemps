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
	reset();

	while(true) {
		// @todo Save pc on mem_pause ??
		// @todo Inform externally of current page ??

		if(reset_in.read()){
			reset();
			continue;
		}

		mip.MEI() = intr_in.read();
		if(handle_interrupts())	// If interrupt is handled, continues to next PC
			continue;

		if(fetch())	// If exception occurred, continues to next PC
			continue;

	}

}

void RiscV::reset()
{
	priv.set(Privilege::Level::MACHINE);
	mstatus.MIE() = 0;
	mstatus.MPRV() = 0;
	// misa = full capabilities, not implemented
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
