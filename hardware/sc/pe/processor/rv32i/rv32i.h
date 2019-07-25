/**
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * UPF - Universidade de Passo Fundo (upf.br)
 * 
 * @brief
 * Header file for the RISC-V RV32I Instruction Set Simulator (ISS)
 */

#pragma once

#include <systemc.h>

#include <stdint.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <assert.h>
//#include <math.h>
//#include "../../../standards.h"

static const int FLIT_SIZE = 32;
static const int HALF_FLIT = FLIT_SIZE/2;

enum class PRIV : uint32_t {
    USER,
    SUPERVISOR,
    RESERVED,
    MACHINE
};

enum class XLEN : uint32_t {
    MXL_32 = 1 << 30
};

enum class EXT : uint32_t {
    A = 1 << 25,
    B = 1 << 24,
    C = 1 << 23,
    D = 1 << 22,
    E = 1 << 21,
    F = 1 << 20,
    G = 1 << 19,
    H = 1 << 18,
    I = 1 << 17,
    J = 1 << 16,
    K = 1 << 15,
    L = 1 << 14,
    M = 1 << 13,
    N = 1 << 12,
    O = 1 << 11,
    P = 1 << 10,
    Q = 1 << 9,
    R = 1 << 8,
    S = 1 << 7,
    T = 1 << 6,
    U = 1 << 5,
    V = 1 << 4,
    W = 1 << 3,
    X = 1 << 2,
    Y = 1 << 1,
    Z = 1 << 0,
};

enum class STATUS : uint32_t {
    UIE = 1 << 0,
    SIE = 1 << 1,
    MIE = 1 << 3,

    UPIE = 1 << 4,
    SPIE = 1 << 5,
    MPIE = 1 << 7,

    SPP_SHIFT = 8,
    MPP_SHIFT = 11, 

    MPRV = 1 << 17,

    MXR = 1 << 19
};

typedef sc_uint<HALF_FLIT> half_flit_t;

// Processor state
typedef struct _state {
	uint32_t x[32]; // 32 integer registers. Embedded version has only 16. x[0] = 0.
                    // The size of the register is XLEN. The RISC-V GPRs are called "x" registers.
	uint32_t pc;    // Program counter

   uint32_t global_inst_reg; // ?
   uint32_t epc;     // Exception program counter. Does it exists in RISC-V?
} state_t;




SC_MODULE(Rv32i){

	sc_in<bool> clk;
	sc_in<bool> reset;
	sc_in<bool> mem_pause;
    sc_in<bool> intr_in;
	
    sc_in < sc_uint<32> > mem_data_r;
    sc_out< sc_uint<32> > mem_data_w;
    sc_out< sc_uint<4>  > mem_byte_we; // ?
	sc_out< sc_uint<32> > mem_addr;

	sc_out< sc_uint<8> >  current_page;

    sc_uint<4> byte_en; // ?

	state_t *state, state_instance; // Process switching

    struct csr {
        const uint32_t MVENDORID = 0;
        const uint32_t MARCHID = 0;
        const uint32_t MIMPID = 0;
        const uint32_t MHARTID = 0;

        uint32_t mstatus = (uint32_t)STATUS::MIE | (uint32_t)STATUS::MPIE;
        uint32_t misa = (uint32_t)XLEN::MXL_32 | (uint32_t)EXT::I | (uint32_t)EXT::M;

    };

// ISS variables
	

    uint32_t prefetched_opcode, pc_last;
	uint32_t op, rs, rt, rd, re, func, imm, target;
	int32_t  imm_shift;
	int32_t  *r, word_addr;
	uint32_t *u;
	uint32_t ptr, page, byte_write;
	uint8_t  big_endian, shift;

	bool intr_en, prefetch, jmp_or_br;

    unsigned long int pc_count;

    unsigned long int global_inst;
    unsigned long int logical_inst;
    unsigned long int branch_inst;	  
    unsigned long int jump_inst;
    unsigned long int move_inst;
    unsigned long int other_inst;
    unsigned long int arith_inst;
    unsigned long int load_inst;
    unsigned long int shift_inst;
    unsigned long int nop_inst;
    unsigned long int mult_div_inst;

    /* Instructions for PAGE 0 (KERNEL) */
    unsigned long int global_inst_kernel;
    unsigned long int logical_inst_kernel;
    unsigned long int branch_inst_kernel;	  
    unsigned long int jump_inst_kernel;
    unsigned long int move_inst_kernel;
    unsigned long int other_inst_kernel;
    unsigned long int arith_inst_kernel;
    unsigned long int load_inst_kernel;
    unsigned long int shift_inst_kernel;
    unsigned long int nop_inst_kernel;
    unsigned long int mult_div_inst_kernel;	  


    /* Instructions for PAGES different from 0 (TASKS) */
    unsigned long int global_inst_tasks;
    unsigned long int logical_inst_tasks;
    unsigned long int branch_inst_tasks;	  
    unsigned long int jump_inst_tasks;
    unsigned long int move_inst_tasks;
    unsigned long int other_inst_tasks;
    unsigned long int arith_inst_tasks;
    unsigned long int load_inst_tasks;
    unsigned long int shift_inst_tasks;
    unsigned long int nop_inst_tasks;
    unsigned long int mult_div_inst_tasks;
 
	/*** Process function ***/
	void rv32i_execution();

	/*** Helper functions ***/ //Needed?
	void mult_big(unsigned int a, unsigned int b);
	void mult_big_signed(int a, int b);
	
	SC_HAS_PROCESS(rv32i_execution);
	Rv32i(sc_module_name name_, half_flit_t router_addr_ = 0) :
	sc_module(name_), router_addr(router_addr_)
	{
		SC_THREAD(rv32i_execution);
		sensitive << clk.pos() << mem_pause.pos();
		sensitive << mem_pause.neg();

		state = &state_instance;

		// MIPS: Big endian.
        // RISCV: Little endian.
		big_endian = 0;

		// Used to generate the 'current_page' signal from the 'page' signal
		shift = (unsigned char)(log10(PAGE_SIZE_BYTES)/log10(2));

        pc_count = 0;
	
        logical_inst				= 0;
        jump_inst					= 0;
        branch_inst					= 0;
        move_inst					= 0;
        other_inst					= 0;
        arith_inst					= 0;
        load_inst					= 0;
        shift_inst					= 0;	
        nop_inst					= 0;	
        mult_div_inst				= 0;
        
        /* Instructions for PAGE 0 (KERNEL) */
        global_inst_kernel			= 0;
        logical_inst_kernel			= 0;
        branch_inst_kernel			= 0;	
        jump_inst_kernel			= 0;
        move_inst_kernel   			= 0;
        other_inst_kernel 			= 0;
        arith_inst_kernel  			= 0;
        load_inst_kernel   			= 0;
        shift_inst_kernel  			= 0;	
        nop_inst_kernel    			= 0;	
        mult_div_inst_kernel   		= 0;

        /* Instructions for PAGES different from 0 (TASKS) */
        global_inst_tasks			= 0;
        logical_inst_tasks   		= 0;
        jump_inst_tasks   			= 0;
        branch_inst_tasks   		= 0;
        move_inst_tasks      		= 0;
        other_inst_tasks   			= 0;
        arith_inst_tasks    		= 0;
        load_inst_tasks    			= 0;
        shift_inst_tasks     		= 0;	
        nop_inst_tasks     			= 0;	
        mult_div_inst_tasks     	= 0;
    }

private:
    static const uint8_t OP = 0x33;

//  CPU registers
    uint32_t instr; // base RISC-V ISA has fixed-length 32-bit instructions
    uint8_t opcode;
    uint8_t rs1, rs2, rd, funct3, funct7;




    half_flit_t router_addr;
    void get_opcode();

};
