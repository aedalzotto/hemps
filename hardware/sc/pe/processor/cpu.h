/**
 * @file cpu.h
 * 
 * @author
 * Angelo Elias Dalzotto (150633@upf.br)
 * UPF - Universidade de Passo Fundo (upf.br)
 * 
 * @brief
 * Generic CPU class
 */

#pragma once

#include <systemc.h>
#include <stdint.h>
#include "../../standards.h"

/* Length of a flit */
static const int FLIT_SIZE = 32;
static const int HALF_FLIT = FLIT_SIZE/2;
typedef sc_uint<HALF_FLIT> half_flit_t;

class CPU : public sc_module {
public:
    CPU(sc_module_name name_, half_flit_t router_addr_ = 0) : sc_module(name_), router_addr(router_addr_) {}

    /**
	 * Module ports
	 * All we be mantained to keep compatibility with HeMPS framework.
	 */
	sc_in_clk	clk;
	sc_in<bool> reset_in;
	sc_in<bool> intr_in;

	sc_out<sc_uint<32> > mem_address;	// Memory address bus
	sc_in<sc_uint<32> >  mem_data_r;	// Memory data read bus
	sc_out<sc_uint<4> >  mem_byte_we;	// Memory data write byte select
	sc_out<sc_uint<32> > mem_data_w;	// Memory data write bus

	/* Not in use. Declared for compatibility reasons */
	sc_in<bool> mem_pause;

	/* Don't know if in use */
	sc_out<sc_uint<8> > current_page;

    /* Use for statistics */
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
    
protected:
    /* PE router address. Used by the simulator */
	half_flit_t router_addr;
};