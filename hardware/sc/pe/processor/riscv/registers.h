/**
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * UPF - Universidade de Passo Fundo (upf.br)
 * 
 * @brief
 * Risc-V registers class definitions.
 */

#pragma once

#include <systemc.h>

/* Length of CPU registers */
static const int XLEN = 32;

/* RISC-V instruction format */
namespace instruction {
	sc_uint<XLEN> reg;

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

/* Supervisor Address Translation and Protection (satp) Register */
namespace satp {
	sc_uint<XLEN> reg;

	sc_dt::sc_uint_bitref MODE() { return reg.bit(31); }
	sc_dt::sc_uint_subref ASID() { return reg.range(30, 22); }
	sc_dt::sc_uint_subref PPN() { return reg.range(21, 0); }
};

namespace va {
	sc_uint<XLEN> reg;

	sc_dt::sc_uint_subref VPN(bool level) { return level ? reg.range(31, 22) : reg.range(21, 12); }
	sc_dt::sc_uint_subref page_offset() { return reg.range(11, 0); }
};

namespace pa {
	sc_uint<XLEN> reg;

	sc_dt::sc_uint_subref PPN(bool level) { return level ? reg.range(31, 22) : reg.range(21, 12); }
	sc_dt::sc_uint_subref page_offset() { return reg.range(11, 0); }
};