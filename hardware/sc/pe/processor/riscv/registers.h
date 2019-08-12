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