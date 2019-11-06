//------------------------------------------------------------------------------------------------
//
//  DISTRIBUTED HEMPS -  5.0
//
//  Research group: GAPH-PUCRS    -    contact   fernando.moraes@pucrs.br
//
//  Distribution:  September 2013
//
//  Source name:  mlite_cpu.h
//
//  Brief description: This source has the module mlite_cpu used in mlite_cpu.cpp
//
//------------------------------------------------------------------------------------------------

#ifndef _mlite_cpu_h
#define _mlite_cpu_h

#include <systemc.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "../../../standards.h"
#include "../cpu.h"

typedef struct {
   long int r[32];
   unsigned long int pc, epc, global_inst_reg;
   long int hi;
   long int lo;
} State;

class mlite_cpu : public CPU {
public:
	/*** Process function ***/
	void mlite();

	/*** Helper functions ***/
	void mult_big(unsigned int a, unsigned int b);
	void mult_big_signed(int a, int b);
	
	SC_HAS_PROCESS(mlite_cpu);
	mlite_cpu(sc_module_name name_, half_flit_t address_router_ = 0) :
	CPU(name_, address_router_)
	{

		SC_THREAD(mlite);
		sensitive << clk.pos() << mem_pause.pos();
		sensitive << mem_pause.neg();

		state = &state_instance;

		// MIPS: Big endian.
		big_endian = 1;

		// Used to generate the 'current_page' signal from the 'page' signal
		shift = (unsigned char) (log10(PAGE_SIZE_BYTES)/log10(2));
	}
private:
	sc_uint<4> byte_en;
	State *state, state_instance;

	unsigned int opcode, prefetched_opcode, pc_last;
	unsigned int op, rs, rt, rd, re, func, imm, target;
	int imm_shift;
	int *r, word_addr;
	unsigned int *u;
	unsigned int ptr, page, byte_write;
	unsigned char big_endian, shift;

	bool intr_enable, prefetch, jump_or_branch, no_execute_branch_delay_slot;
};

#endif
