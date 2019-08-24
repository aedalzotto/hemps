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
	 * @brief Load a register from virtual/physical memory
	 * 
	 * @param address The memory address to load from.
	 * 
	 * @return True if exception occurred
	 */
	bool load(sc_uint<34> address);

	/**
	 * @brief Store a register to virtual/physical memory
	 * 
	 * @param address The memory address to store to.
	 * 
	 * @return True if exception occurred
	 */
	bool store(sc_uint<34> address);

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
	 * 
	 * @return False
	 */
	bool lui();

	/**
	 * @brief Add Upper Immediate to PC.
	 * 
	 * @detail 	rd ← pc + offset
	 * 
	 * @return False
	 */
	bool auipc();

	/**
	 * @brief Jump and Link
	 * 
	 * @detail rd ← pc + length(inst)
	 * 		   pc ← pc + offset
	 * 
	 * @return True. Can generate Instruction Address Misaligned.
	 */
	bool jal();

	/**
	 * @brief Jump and Link Register
	 * 
	 * @detail rd ← pc + length(inst)
	 * 		   pc ← (rs1 + offset) ∧ -2
	 * 
	 * @return True. Can generate Instruction Address Misaligned.
	 */
	bool jalr();

	/**
	 * @brief Branch Equal
	 * 
	 * @detail if rs1 = rs2 then pc ← pc + offset
	 * 
	 * @return False if branch not taken.
	 * 		   True if branch taken. Can generate Instruction Address Misaligned.
	 */
	bool beq();

	/**
	 * @brief Branch Not Equal
	 * 
	 * @detail if rs1 ≠ rs2 then pc ← pc + offset
	 * 
	 * @return False if branch not taken.
	 * 		   True if branch taken. Can generate Instruction Address Misaligned.
	 */
	bool bne();

	/**
	 * @brief Branch Less Than
	 * 
	 * @detail if rs1 < rs2 then pc ← pc + offset
	 * 
	 * @return False if branch not taken.
	 * 		   True if branch taken. Can generate Instruction Address Misaligned.
	 */
	bool blt();

	/**
	 * @brief Branch Greater than Equal
	 * 
	 * @detail if rs1 ≥ rs2 then pc ← pc + offset
	 * 
	 * @return False if branch not taken.
	 * 		   True if branch taken. Can generate Instruction Address Misaligned.
	 */
	bool bge();

	/**
	 * @brief Branch Less Than Unsigned
	 * 
	 * @detail if rs1 < rs2 then pc ← pc + offset
	 * 
	 * @return False if branch not taken.
	 * 		   True if branch taken. Can generate Instruction Address Misaligned.
	 */
	bool bltu();

	/**
	 * @brief Branch Greater than Equal Unsigned
	 * 
	 * @detail if rs1 ≥ rs2 then pc ← pc + offset
	 * 
	 * @return False if branch not taken.
	 * 		   True if branch taken. Can generate Instruction Address Misaligned.
	 */
	bool bgeu();

	/**
	 * @brief Load Byte
	 * 
	 * @detail rd ← s8[rs1 + offset]
	 * 
	 * @return False.
	 */
	bool lb();
	bool lh();
	bool lw();
	bool lbu();
	bool lhu();
	bool sb();
	bool sh();
	bool sw();

	/**
	 * @brief Add Immediate
	 * 
	 * @detail rd ← rs1 + sx(imm)
	 * 
	 * @return False.
	 */
	bool addi();

	/**
	 * @brief Set Less Than Immediate
	 * 
	 * @detail rd ← sx(rs1) < sx(imm)
	 * 
	 * @return False.
	 */
	bool slti();

	/**
	 * @brief Set Less Than Immediate Unsigned
	 * 
	 * @detail rd ← ux(rs1) < ux(imm)
	 * 
	 * @return False.
	 */
	bool sltiu();

	/**
	 * @brief Xor Immediate
	 * 
	 * @detail rd ← ux(rs1) ⊕ ux(imm)
	 * 
	 * @return False.
	 */
	bool xori();

	/**
	 * @brief Or Immediate
	 * 
	 * @detail rd ← ux(rs1) ∨ ux(imm)
	 * 
	 * @return False.
	 */
	bool ori();

	/**
	 * @brief And Immediate
	 * 
	 * @detail rd ← ux(rs1) ∧ ux(imm)
	 * 
	 * @return False.
	 */
	bool andi();

	/**
	 * @brief Shift Left Logical Immediate
	 * 
	 * @detail rd ← ux(rs1) ∧ ux(imm)
	 * 
	 * @return False.
	 */
	bool slli();

	/**
	 * @brief Shift Right Logical Immediate
	 * 
	 * @detail rd ← ux(rs1) » ux(imm)
	 * 
	 * @return False.
	 */
	bool srli();

	/**
	 * @brief Shift Right Arithmetic Immediate
	 * 
	 * @detail rd ← sx(rs1) » ux(imm)
	 * 
	 * @return False.
	 */
	bool srai();

	/**
	 * @brief Add
	 * 
	 * @detail rd ← sx(rs1) + sx(rs2)
	 * 
	 * @return False.
	 */
	bool add();

	/**
	 * @brief Subtract
	 * 
	 * @detail rd ← sx(rs1) - sx(rs2)
	 * 
	 * @return False.
	 */
	bool sub();

	/**
	 * @brief Shift Left Logical
	 * 
	 * @detail rd ← ux(rs1) « rs2
	 * 
	 * @return False.
	 */
	bool sll();

	/**
	 * @brief Set Less Than
	 * 
	 * @detail rd ← sx(rs1) < sx(rs2)
	 * 
	 * @return False.
	 */
	bool slt();

	/**
	 * @brief Set Less Than Unsigned
	 * 
	 * @detail rd ← ux(rs1) < ux(rs2)
	 * 
	 * @return False.
	 */
	bool sltu();

	/**
	 * @brief Xor
	 * 
	 * @detail rd ← ux(rs1) ⊕ ux(rs2)
	 * 
	 * @return False.
	 */
	bool _xor();

	/**
	 * @brief Shift Right Logical
	 * 
	 * @detail rd ← ux(rs1) » rs2
	 * 
	 * @return False.
	 */
	bool srl();

	/**
	 * @brief Shift Right Arithmetic
	 * 
	 * @detail rd ← sx(rs1) » rs2
	 * 
	 * @return False.
	 */
	bool sra();

	/**
	 * @brief Or
	 * 
	 * @detail rd ← ux(rs1) ∨ ux(rs2)
	 * 
	 * @return False.
	 */
	bool _or();

	/**
	 * @brief And
	 * 
	 * @detail rd ← ux(rs1) ∧ ux(rs2)
	 * 
	 * @return False.
	 */
	bool _and();

	/**
	 * @brief Fence
	 * 
	 * @detail Implemented as NOP.
	 * 
	 * @return False.
	 */
	bool fence();

	/**
	 * @brief Service Request to the Execution Environment
	 * 
	 * @return True.
	 */
	bool ecall();

	/**
	 * @brief Return Control to a Debugging Environment
	 * 
	 * @detail Implemented as NOP
	 * 
	 * @return False.
	 */
	bool ebreak();

	/**
	 * @brief Multiply
	 * 
	 * @detail rd ← ux(rs1) × ux(rs2)
	 * 
	 * @return False.
	 */
	bool mul();
	
	/**
	 * @brief Multiply High Signed Signed
	 * 
	 * @detail rd ← (sx(rs1) × sx(rs2)) » xlen
	 * 
	 * @return False.
	 */
	bool mulh();
	
	/**
	 * @brief Multiply High Signed Unsigned
	 * 
	 * @detail rd ← (sx(rs1) × ux(rs2)) » xlen
	 * 
	 * @return False.
	 */
	bool mulhsu();
	
	/**
	 * @brief Multiply High Unsigned Unsigned
	 * 
	 * @detail rd ← (ux(rs1) × ux(rs2)) » xlen
	 * 
	 * @return False.
	 */
	bool mulhu();
	
	/**
	 * @brief Divide Signed
	 * 
	 * @detail rd ← sx(rs1) ÷ sx(rs2)
	 * 
	 * @return False.
	 */
	bool div();
	
	/**
	 * @brief Divide Unsigned
	 * 
	 * @detail rd ← ux(rs1) ÷ ux(rs2)
	 * 
	 * @return False.
	 */
	bool divu();
	
	/**
	 * @brief Remainder Signed
	 * 
	 * @detail rd ← sx(rs1) mod sx(rs2)
	 * 
	 * @return False.
	 */
	bool rem();
	
	/**
	 * @brief Remainder Unsigned
	 * 
	 * @detail rd ← ux(rs1) mod ux(rs2)
	 * 
	 * @return False.
	 */
	bool remu();

	/**
	 * @brief Return from trap under S-Mode
	 * 
	 * @return True. Can raise exception.
	 */
	bool sret();

	/**
	 * @brief Return from trap under M-Mode
	 * 
	 * @return True. Can raise exception.
	 */
	bool mret();


	/**
	 * @brief Wait for Interrupt
	 * 
	 * @return True if TW=1 in mstatus, generating exception.
	 */
	bool wfi();

	/**
	 * @brief Supervisor memory-management fence
	 * 
	 * @detail Implemented as NOP
	 * 
	 * @return False.
	 */
	bool sfence_vma();

	/**
	 * @brief Pointer to execute function that will be set by the decoder.
	 * 
	 * @return True if jump happened by branch, jump or exception.
	 */
	bool (RiscV::*execute)();
	
};