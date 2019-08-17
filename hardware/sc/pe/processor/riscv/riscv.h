/**
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * UPF - Universidade de Passo Fundo (upf.br)
 * 
 * @brief
 * Header file for the RISC-V RV32I Instruction Set Simulator (ISS)
 */

#pragma once

#include "registers.h"

#include <systemc.h>
#include <stdint.h>

/* Length of a flit */
static const int FLIT_SIZE = 32;
static const int HALF_FLIT = FLIT_SIZE/2;
typedef sc_uint<HALF_FLIT> half_flit_t;

namespace vectors {
	const uint32_t RESET = 0;
};

SC_MODULE(RiscV) {
public:
	SC_HAS_PROCESS(RiscV);

	/**
	 * Module ports
	 * All we be mantained to keep compatibility with HeMPS framework.
	 */
	sc_in_clk	clock;
	sc_in<bool> reset_in;
	sc_in<bool> intr_in;

	sc_out<sc_uint<32> > mem_address;
	sc_in<sc_uint<32> >  mem_data_r;

	/**
	 * @brief The loop of the RISC-V CPU.
	 * 
	 * @details Fetch, decode, execute, write-back
	 */
	void cpu();

	/**
	 * @brief SC_MODULE constructor.
	 * 
	 * @param name_ The SC_MODULE name
	 * @param router_addr_ The PE router address.
	 */
	RiscV(sc_module_name name_, half_flit_t router_addr_ = 0);

private:
	/* GPRs "X" registers */
	Register x[32];

	/* Program counter */
	Address pc;

	/* This is not a regular register. It just keeps track of the current privilege level */
	Privilege priv;

	/* Fetched instruction register */
	Instruction instr;

	/**
	 * Machine-level CSRs
	 * 
	 * MACHINE INFORMATION REGISTERS
	 * mvendorid:	Vendor ID.
	 * marchid:		Architecture ID.
	 * mimpid:		Implementation ID.
	 * mhartid:		Hardware thread ID.
	 * 
	 * MACHINE TRAP SETUP
	 * mstatus:		Machine status register.
	 * misa:		ISA and extensions.
	 * medeleg:		Machine exception delegation register.
	 * mideleg:		Machine interrupt delegation register.
	 * mie:			Machine interrupt-enable register.
	 * mtvec:		Machine trap-handler base address.
	 * mcounteren:	Machine conter enable.
	 * 
	 * MACHINE TRAP HANDLING
	 * mscratch:	Scratch register for machine trap handlers
	 * mepc:		Machine exception program counter.
	 * mcause:		Machine trap cause.
	 * mtval:		Machine bad address or instruction.
	 * mip:			Machine interrupt pending.
	 */
	const register_t mvendorid = 0;
	const register_t marchid = 0;
	const register_t mimpid = 0;
	const register_t mhartid = 0;
	Mstatus mstatus;
	ISA::Misa misa;
	Exceptions::Mer medeleg;
	Interrupts::Mir mideleg;
	Interrupts::Mir mie;
	Mtvec mtvec;
	//mcounteren
	//mscratch
	Address mepc;
	Mcause mcause;
	Register mtval;
	Interrupts::Mir mip;

	/**
	 * Supervisor-level CSRs
	 * 
	 * SUPERVISOR TRAP SETUP
	 * sstatus:		Supervisor status register.
	 * sedeled:		Supervisor exception delegation register.
	 * sideleg:		Supervisor interrupt delegation register.
	 * sie:			Supervisor interrupt-enable register.
	 * stvec:		Supervisor trap handler base address.
	 * scounteren:	Supervisor counter enable.
	 * 
	 * SUPERVISOR TRAP HANDLING
	 * sscratch:	Supervisor register for supervisor trap handlers.
	 * sepc:		Supervisor exception program counter.
	 * scause:		Supervisor trap cause.
	 * stval:		Supervisor bad address or instruction.
	 * sip:			Supervisor interrupt pending.
	 * 
	 * SUPERVISOR PROTECTION AND TRANSLATION
	 * satp:		Supervisor address translation and protection.
	 */
	//sstatus
	//sedeleg
	//sideleg
	//sie
	Mtvec stvec;
	//scounteren
	//sscratch
	Address sepc;
	Mcause scause;
	Register stval;
	//sip
	Satp satp;

	/* PE router address. Used by the simulator */
	half_flit_t router_addr;

	/**
	 * @brief Resets the CPU to its initial state.
	 */
	void reset();

	/**
	 * @brief Handle pending interrupts.
	 * 
	 * @detail If interrupt is not masked, take it.
	 * 
	 * @return True if interrupt has been taken.
	 */
	bool handle_interrupts();

	/**
	 * @brief Fetches an instruction from virtual/physical memory
	 * 
	 * @return True if exception occurred
	 */
	bool fetch();

	/**
	 * @brief Reads XLEN from memory.
	 * 
	 * @param address The memory address to be readed.
	 * 
	 * @return XLEN bits from memory address.
	 */
	register_t mem_read(sc_uint<34> address);

	/**
	 * @brief Handles synchronous exceptions
	 */
	void handle_exceptions(Exceptions::CODE code);

	/**
	 * @brief Decodes and executes the fetched instruction.
	 * 
	 * @return True if exception occurred
	 */
	bool decode();

	/**
	 * @brief Decodes the OP-IMM opcode.
	 * 
	 * @return True if exception occurred
	 */
	bool decode_op_imm();

	/**
	 * @brief Decodes the OP opcode.
	 * 
	 * @return True if exception occurred
	 */
	bool decode_op();

	/**
	 * @brief Decodes the BRANCH opcode.
	 * 
	 * @return True if exception occurred
	 */
	bool decode_branch();

	/**
	 * @brief Decodes the LOAD opcode.
	 * 
	 * @return True if exception occurred
	 */
	bool decode_load();

	/**
	 * @brief Decodes the LOAD opcode.
	 * 
	 * @return True if exception occurred
	 */
	bool decode_store();

	/**
	 * @brief Decodes the LOAD opcode.
	 * 
	 * @return True if exception occurred
	 */
	bool decode_system();

	// RV32I Instructions
	/**
	 * @brief Load Upper Immediate.
	 * 
	 * @detail rd ← imm
	 */
	void lui();

	/**
	 * @brief Add Upper Immediate to PC.
	 * 
	 * @detail rd ← imm
	 */
	void auipc();
	void jal();
	void jalr();
	void beq();
	void bne();
	void blt();
	void bge();
	void bltu();
	void bgeu();
	void lb();
	void lh();
	void lw();
	void lbu();
	void lhu();
	void sb();
	void sh();
	void sw();
	void addi();
	void slti();
	void sltiu();
	void xori();
	void ori();
	void andi();
	void slli();
	void srli();
	void srai();
	void add();
	void sub();
	void sll();
	void slt();
	void sltu();
	void _xor();
	void srl();
	void sra();
	void _or();
	void _and();
	void fence();
	void ecall();
	void ebreak();

	/**
	 * @brief Pointer to execute function that will be set by the decoder.
	 */
	void (RiscV::*execute)();
	
};