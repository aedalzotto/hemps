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
    sc_in<bool> mem_pause;

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

    /**
     * Machine-level CSRs
     * 
     * MACHINE INFORMATION REGISTERS
     * mvendorid:   Vendor ID.
     * marchid:     Architecture ID.
     * mimpid:      Implementation ID.
     * mhartid:     Hardware thread ID.
     * 
     * MACHINE TRAP SETUP
     * mstatus:     Machine status register.
     * misa:        ISA and extensions.
     * medeleg:     Machine exception delegation register.
     * mideleg:     Machine interrupt delegation register.
     * mie:         Machine interrupt-enable register.
     * mtvec:       Machine trap-handler base address.
     * mcounteren:  Machine conter enable.
     * 
     * MACHINE TRAP HANDLING
     * mscratch
     * mepc
     * mcause:
     * mtval
     * mip
     */
    const register_t mvendorid = 0;
    const register_t marchid = 0;
    const register_t mimpid = 0;
    const register_t mhartid = 0;
    Mstatus mstatus;
    const register_t misa = 0;

    Interrupts::Mir mideleg;
    Interrupts::Mir mie;
    Mtvec mtvec;


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

	void reset();
    bool handle_interrupts();
	Instruction fetch();
    Register mem_read(sc_uint<34> Address);

    Register vatp();

};


/*
public:
    struct csr {
        const uint32_t MVENDORID = 0;
        const uint32_t MARCHID = 0;
        const uint32_t MIMPID = 0;
        const uint32_t MHARTID = 0;

        uint32_t mstatus = (uint32_t)STATUS::MIE | (uint32_t)STATUS::MPIE;
        uint32_t misa = (uint32_t)XLEN::MXL_32 | (uint32_t)EXT::I | (uint32_t)EXT::M;
        uint32_t medeleg;
        uint32_t mideleg;
        uint32_t mie;
        uint32_t mtvec = TRAP_BASE_ADDR << TRAP_BASE_SHIFT | (uint32_t)TRAP_MODE::DIRECT;
        uint32_t mcounteren;

        uint32_t mscratch;
        uint32_t mepc;
        uint32_t mip;

        uint64_t mcycle;
        uint64_t minstret;
        //mhpmcounter3-31 = 0

        //mcountinhibit = 0
        //mhpmevent3-31 = 0

    };
*/
/*
static const uint32_t TRAP_BASE_SHIFT = 2;

enum class PRIV : uint32_t {
    USER,
    SUPERVISOR,

    MACHINE = 3
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

    SPP = 1 << 8,
    MPP_SHIFT = 11, 

    MPRV = 1 << 17,

    MXR = 1 << 19,
    SUM = 1 << 18,

    TVM = 1 << 20,
    TW  = 1 << 21,
    TSR = 1 << 22,

    FS_SHIFT = 13,
    XS_SHIFT = 15,

    SD = 1 << 31
};

enum class TRAP_MODE : uint32_t {
    DIRECT,
    VECTORED
};

enum class INTR_TYPE : uint32_t {
    USI,
    SSI,
    MSI=3,

    UTI,
    STI,
    MTI=7,

    UEI,
    SEI,
    MEI=11
};

enum EXC_TYPE : uint32_t {
    IAM,
    IAF,
    II,
    BP,
    LAM,
    LAF,
    SAM,
    SAF,
    ECU,
    ECS,
    RES,
    ECM,
    IPF,
    LPF,
    RES_STD_FUT,
    SPF
};

enum HPM : uint32_t {
    CY,
    TM,
    IR,
    HPM3
};
 */