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

class Register {
protected:
	register_t reg;
public:
	void write(register_t value) { reg = value; }
	register_t read() { return reg; }
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

namespace Sv32 {
	const uint32_t PAGESIZE = 4096;
	const uint32_t LEVELS = 2;
	const uint32_t PTESIZE = 4;

	class VirtualAddress : public Address {
	public:
		sc_dt::sc_uint_subref VPN(int level) { return level ? reg.range(31, 22) : reg.range(21, 12); }
	};

	class PhysicalAddress : public Address {
	private:
		sc_uint<34> reg;
	public:
		sc_dt::sc_uint_subref PPN(int level) { return level ? reg.range(33, 22) : reg.range(21, 12); }
	};

	class PageTableEntry : public Register {
	public:
		sc_dt::sc_uint_subref PPN(int level) { return level ? reg.range(31, 20) : reg.range(19, 10); }

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
	sc_dt::sc_uint_bitref MODE() { return reg.bit(31); }
	sc_dt::sc_uint_subref ASID() { return reg.range(30, 22); }
	sc_dt::sc_uint_subref PPN() { return reg.range(21, 0); }
};