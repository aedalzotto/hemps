/**
 * @file registers.h
 * 
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * UPF - Universidade de Passo Fundo (upf.br)
 * 
 * @brief This file describes the registers and address formats for the 
 * 32-bit XLEN Risc-V.
 */


#pragma once

#include <systemc.h>

static const uint32_t XLEN = 32;
typedef sc_uint<XLEN> register_t;

class Privilege {
public:
	enum class Level : uint8_t {
		USER,
		SUPERVISOR,
		MACHINE = 3
	};

private:
	Level reg;

public:
	void set(Level level) { reg = level; }
	Level get() { return reg; };
};

class Register {
protected:
	register_t reg;
public:
	Register();
	Register(register_t reg_) : reg(reg_) { }
	Register(const Register &Reg) : reg(Reg.reg) { }
	void write(register_t value) { reg = value; }
	register_t read() { return reg; }
};

namespace ISA {
	enum Ext : uint32_t {
		A = 1 << 0,
		B = 1 << 1,
		C = 1 << 2,
		D = 1 << 3,
		E = 1 << 4,
		F = 1 << 5,
		G = 1 << 6,
		H = 1 << 7,
		I = 1 << 8,
		J = 1 << 9,
		K = 1 << 10,
		L = 1 << 11,
		M = 1 << 12,
		N = 1 << 13,
		P = 1 << 15,
		Q = 1 << 16,
		S = 1 << 18,
		T = 1 << 19,
		U = 1 << 20,
		V = 1 << 21,
		X = 1 << 22
	};

	class Misa : public Register {
	public:
		sc_dt::sc_uint_bitref X() { return reg.bit(22); }
		sc_dt::sc_uint_bitref V() { return reg.bit(21); }
		sc_dt::sc_uint_bitref U() { return reg.bit(20); }
		sc_dt::sc_uint_bitref T() { return reg.bit(19); }
		sc_dt::sc_uint_bitref S() { return reg.bit(18); }
		sc_dt::sc_uint_bitref Q() { return reg.bit(16); }
		sc_dt::sc_uint_bitref P() { return reg.bit(15); }
		sc_dt::sc_uint_bitref N() { return reg.bit(13); }
		sc_dt::sc_uint_bitref M() { return reg.bit(12); }
		sc_dt::sc_uint_bitref L() { return reg.bit(11); }
		sc_dt::sc_uint_bitref K() { return reg.bit(10); }
		sc_dt::sc_uint_bitref J() { return reg.bit(9); }
		sc_dt::sc_uint_bitref I() { return reg.bit(8); }
		sc_dt::sc_uint_bitref H() { return reg.bit(7); }
		sc_dt::sc_uint_bitref G() { return reg.bit(6); }
		sc_dt::sc_uint_bitref F() { return reg.bit(5); }
		sc_dt::sc_uint_bitref E() { return reg.bit(4); }
		sc_dt::sc_uint_bitref D() { return reg.bit(3); }
		sc_dt::sc_uint_bitref C() { return reg.bit(2); }
		sc_dt::sc_uint_bitref B() { return reg.bit(1); }
		sc_dt::sc_uint_bitref A() { return reg.bit(0); }
	};
};

class Mstatus : public Register {
private:
	static const uint32_t MASK = 0x807FF9BB;
public:
	void write(register_t value) { reg = (value & MASK);  }
	register_t read() { return (reg & MASK); }

	sc_dt::sc_uint_bitref SD() { return reg.bit(31); }
	sc_dt::sc_uint_bitref TSR() { return reg.bit(22); }
	sc_dt::sc_uint_bitref TW() { return reg.bit(21); }
	sc_dt::sc_uint_bitref TVM() { return reg.bit(20); }
	sc_dt::sc_uint_bitref MXR() { return reg.bit(19); }
	sc_dt::sc_uint_bitref SUM() { return reg.bit(18); }
	sc_dt::sc_uint_bitref MPRV() { return reg.bit(17); }
	sc_dt::sc_uint_subref XS() { return reg.range(16, 15); }
	sc_dt::sc_uint_subref FS() { return reg.range(14, 13); }
	sc_dt::sc_uint_subref MPP() { return reg.range(12, 11); }
	sc_dt::sc_uint_bitref SPP() { return reg.bit(8); }
	sc_dt::sc_uint_bitref MPIE() { return reg.bit(7); }
	sc_dt::sc_uint_bitref SPIE() { return reg.bit(5); }
	sc_dt::sc_uint_bitref UPIE() { return reg.bit(4); }
	sc_dt::sc_uint_bitref MIE() { return reg.bit(3); }
	sc_dt::sc_uint_bitref SIE() { return reg.bit(1); }
	sc_dt::sc_uint_bitref UIE() { return reg.bit(0); }
};

// @todo Misa. Hard because has some WARL

class Mcause : public Register {
public:
	sc_dt::sc_uint_bitref interrupt() { return reg.bit(31); }
	sc_dt::sc_uint_subref exception_code() { return reg.range(30, 0); }
};

namespace Interrupts {
	enum CODE {
		USI,
		SSI,
		MSI = 3,

		UTI,
		STI,
		MTI = 7,

		UEI,
		SEI,
		MEI = 11
	};
	enum BIT {
		USI = 1 << CODE::USI,
		SSI = 1 << CODE::SSI,
		MSI = 1 << CODE::MSI,

		UTI = 1 << CODE::UTI,
		STI = 1 << CODE::STI,
		MTI = 1 << CODE::MTI,

		UEI = 1 << CODE::UEI,
		SEI = 1 << CODE::SEI,
		MEI = 1 << CODE::MEI
	};
	enum MODE {
		USER = BIT::USI | BIT::UTI | BIT::UEI,
		SUPERVISOR = BIT::SSI | BIT::STI | BIT::SEI,
		MACHINE = BIT::MSI | BIT::MTI | BIT::MEI
	};

	class Mir : public Register {
	private:
		static const uint32_t MASK = 0xAAA;
	public:
		void write(register_t value) { reg = (value & MASK);  }
		register_t read() { return (reg & MASK); }

		sc_dt::sc_uint_bitref MEI() { return reg.bit(11); }
		sc_dt::sc_uint_bitref SEI() { return reg.bit(9); }
		//sc_dt::sc_uint_bitref UEI() { return reg.bit(8); }
		sc_dt::sc_uint_bitref MTI() { return reg.bit(7); }
		sc_dt::sc_uint_bitref STI() { return reg.bit(5); }
		//sc_dt::sc_uint_bitref UTI() { return reg.bit(4); }
		sc_dt::sc_uint_bitref MSI() { return reg.bit(3); }
		sc_dt::sc_uint_bitref SSI() { return reg.bit(1); }
		//sc_dt::sc_uint_bitref USI() { return reg.bit(0); }
	};
};

namespace Exceptions {
	enum CODE {
		INSTRUCTION_ADDRESS_MISALIGNED,
		INSTRUCTION_ACCESS_FAULT,
		ILLEGAL_INSTRUCTION,
		BREAKPOINT,
		LOAD_ADDRESS_MISALIGNED,
		LOAD_ACCESS_FAULT,
		STORE_AMO_ADDRESS_MISALIGNED,
		STORE_AMO_ACCESS_FAULT,
		ECALL_FROM_UMODE,
		ECALL_FROM_SMODE,

		ECALL_FROM_MMODE = 11,
		INSTRUCTION_PAGE_FAULT,
		LOAD_PAGE_FAULT,

		STORE_AMO_PAGE_FAULT = 15,
		MAX
	};
	enum BIT {
		IAM = 1 << CODE::INSTRUCTION_ADDRESS_MISALIGNED,
		IAF = 1 << CODE::INSTRUCTION_ACCESS_FAULT,
		II  = 1 << CODE::ILLEGAL_INSTRUCTION,
		BP  = 1 << CODE::BREAKPOINT,
		LAM = 1 << CODE::LOAD_ADDRESS_MISALIGNED,
		LAF = 1 << CODE::LOAD_ACCESS_FAULT,
		SAM = 1 << CODE::STORE_AMO_ADDRESS_MISALIGNED,
		SAF = 1 << CODE::STORE_AMO_ACCESS_FAULT,
		ECU = 1 << CODE::ECALL_FROM_UMODE,
		ECS = 1 << CODE::ECALL_FROM_SMODE,
		ECM = 1 << CODE::ECALL_FROM_MMODE,
		IPF = 1 << CODE::INSTRUCTION_PAGE_FAULT,
		LPF = 1 << CODE::LOAD_PAGE_FAULT,
		SPF = 1 << CODE::STORE_AMO_PAGE_FAULT
	};

	class Mer : public Register {
	private:
		static const uint32_t MASK = 0xBFFF;
	public:
		void write(register_t value) { reg = (value & MASK);  }
		register_t read() { return (reg & MASK); }

		sc_dt::sc_uint_bitref SPF() { return reg.bit(14); }
		sc_dt::sc_uint_bitref LPF() { return reg.bit(13); }
		sc_dt::sc_uint_bitref IPF() { return reg.bit(12); }
		//sc_dt::sc_uint_bitref ECM() { return reg.bit(11); }
		sc_dt::sc_uint_bitref ECS() { return reg.bit(9); }
		sc_dt::sc_uint_bitref ECU() { return reg.bit(8); }
		sc_dt::sc_uint_bitref SAF() { return reg.bit(7); }
		sc_dt::sc_uint_bitref SAM() { return reg.bit(6); }
		sc_dt::sc_uint_bitref LAF() { return reg.bit(5); }
		sc_dt::sc_uint_bitref LAM() { return reg.bit(4); }
		sc_dt::sc_uint_bitref BP() { return reg.bit(3); }
		sc_dt::sc_uint_bitref II() { return reg.bit(2); }
		sc_dt::sc_uint_bitref IAF() { return reg.bit(1); }
		sc_dt::sc_uint_bitref IAM() { return reg.bit(0); }
	};
};

/* RISC-V instruction format */
class Instruction : public Register {
public:
	sc_dt::sc_uint_subref opcode() { return reg.range(6, 0); }
	sc_dt::sc_uint_subref rd() { return reg.range(11, 7); }
	sc_dt::sc_uint_subref funct3() { return reg.range(14, 12); }
	sc_dt::sc_uint_subref rs1() { return reg.range(19, 15); }
	sc_dt::sc_uint_subref rs2() { return reg.range(24, 20); }
	sc_dt::sc_uint_subref funct7() { return reg.range(31, 25); }
	sc_dt::sc_uint_subref imm_11_0() { return reg.range(31, 20); }
	sc_dt::sc_uint_subref imm_4_0() { return reg.range(11, 7); }
	sc_dt::sc_uint_subref imm_11_5() { return reg.range(31, 25); }
};

class Address : public Register {
public:
	sc_dt::sc_uint_subref page_offset() { return reg.range(11, 0); }
};

class Mtvec : public Register {
public:
	enum class Mode {
		DIRECT,
		VECTORED
	};
	sc_dt::sc_uint_subref BASE() { return reg.range(31, 2); }
	sc_dt::sc_uint_subref MODE() { return reg.range(1, 0); }
};

namespace Sv32 {
	const uint32_t PAGESIZE = 4096;
	const uint32_t LEVELS = 2;
	const uint32_t PTESIZE = 4;

	class VirtualAddress : public Address {
	public:
		VirtualAddress(Address addr) { reg = addr.read(); }
		sc_dt::sc_uint_subref VPN(int level) { return level ? reg.range(31, 22) : reg.range(21, 12); }
	};

	class PhysicalAddress : public Address {
	private:
		sc_uint<34> reg;
	public:
		PhysicalAddress();
		PhysicalAddress(sc_uint<34> pa) : reg(pa) {};
		sc_dt::sc_uint_subref PPN(int level) { return level ? reg.range(33, 22) : reg.range(21, 12); }
		sc_dt::sc_uint_subref PPN() { return reg.range(33, 12); }
	};

	class PageTableEntry : public Register {
	public:
		sc_dt::sc_uint_subref PPN(int level) { return level ? reg.range(31, 20) : reg.range(19, 10); }
		sc_dt::sc_uint_subref PPN() { return reg.range(31, 10); }

		//sc_dt::sc_uint_subref RSW() { return reg.range(9, 8); }
		sc_dt::sc_uint_bitref D() { return reg.bit(7); }
		sc_dt::sc_uint_bitref A() { return reg.bit(6); }
		sc_dt::sc_uint_bitref G() { return reg.bit(5); }
		sc_dt::sc_uint_bitref U() { return reg.bit(4); }
		sc_dt::sc_uint_bitref X() { return reg.bit(3); }
		sc_dt::sc_uint_bitref W() { return reg.bit(2); }
		sc_dt::sc_uint_bitref R() { return reg.bit(1); }
		sc_dt::sc_uint_bitref V() { return reg.bit(0); }
	};
};

class Satp : public Register {
public:
	enum MODES {
		BARE,
		Sv32
	};
	sc_dt::sc_uint_bitref MODE() { return reg.bit(31); }
	sc_dt::sc_uint_subref ASID() { return reg.range(30, 22); }
	sc_dt::sc_uint_subref PPN() { return reg.range(21, 0); }
};