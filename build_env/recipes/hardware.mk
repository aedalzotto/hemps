COR  =\033[0;32m
NC   =\033[0m # No Color

#systemc g++ compiler
CXX = g++
CXXFLAGS = -g -Wall -O2

HEMPS_TGT=$(notdir $(patsubst %/,%,$(dir $(CURDIR))))

######################################################################################## EDIT:
#SystemC files
TOP 		=hemps test_bench
PE	 		=pe
DMNI 		=dmni
MEMORY 		=ram
RISCV		= riscv/riscv.cpp
MIPSI		= plasma/mlite_cpu.cpp
ROUTER	 	=queue switchcontrol router_cc

TOP_SRC	    	= $(addprefix sc/, 						$(TOP:=.cpp)  $(TOP:=.h) 			)
PE_SRC			= $(addprefix sc/pe/, 					$(PE:=.cpp) $(PE:=.h)  				)
DMNI_SRC		= $(addprefix sc/pe/dmni/, 				$(DMNI:=.cpp) $(DMNI:=.h)			)
MEMORY_SRC		= $(addprefix sc/pe/memory/,			$(MEMORY:=.cpp) $(MEMORY:=.h)		) 
ROUTER_SRC		= $(addprefix sc/pe/router/, 			$(ROUTER:=.cpp) $(ROUTER:=.h)		)
RISCV_SRC		= $(addprefix sc/pe/processor/, 		$(RISCV)							)
MIPSI_SRC		= $(addprefix sc/pe/processor/, 		$(MIPSI)							)
##############################################################################################

PROCESSOR_BASE_DIR = sc/pe/processor
TOP_TGT 		=$(TOP:=.o)
PE_TGT	 		=$(PE:=.o)
DMNI_TGT 		=$(DMNI:=.o)
MEMORY_TGT 		=$(MEMORY:=.o)
RISCV_OBJ 		= $(RISCV_SRC:.cpp=.o)
MIPSI_OBJ 		= $(MIPSI_SRC:.cpp=.o)
ROUTER_TGT	 	=$(ROUTER:=.o)

all: $(HEMPS_TGT)
	@cp $(HEMPS_TGT) ../

$(HEMPS_TGT): $(ROUTER_TGT) $(PROCESSOR_TGT) $(DMNI_TGT) $(MIPSI_OBJ) $(RISCV_OBJ) $(MEMORY_TGT) $(PE_TGT) $(TOP_TGT)
	@printf "${COR}Generating %s ...${NC}\n" "$@"
	@$(CXX) $^ -o $@ -I./ -lsystemc
	
$(TOP_TGT): $(TOP_SRC)
	@printf "${COR}Compiling SystemC source: %s ...${NC}\n" "$(dir $<)$*.cpp"
	@$(CXX) -c $(dir $<)$*.cpp $(CXXFLAGS)

$(PE_TGT): $(PE_SRC)
	@printf "${COR}Compiling SystemC source: %s ...${NC}\n" "$(dir $<)$*.cpp"
	@$(CXX) -c $(dir $<)$*.cpp $(CXXFLAGS)

$(DMNI_TGT): $(DMNI_SRC)
	@printf "${COR}Compiling SystemC source: %s ...${NC}\n" "$(dir $<)$*.cpp"
	@$(CXX) -c $(dir $<)$*.cpp $(CXXFLAGS)

$(MEMORY_TGT): $(MEMORY_SRC)
	@printf "${COR}Compiling SystemC source: %s ...${NC}\n" "$(dir $<)$*.cpp"
	@$(CXX) -c $(dir $<)$*.cpp $(CXXFLAGS)

$(RISCV_OBJ): $(RISCV_SRC)
	@printf "${COR}Compiling SystemC source: %s ...${NC}\n" "$<"
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(MIPSI_OBJ): $(MIPSI_SRC)
	@printf "${COR}Compiling SystemC source: %s ...${NC}\n" "$<"
	$(CXX) -c $< -o $@ $(CXXFLAGS)

$(ROUTER_TGT): $(ROUTER_SRC)
	@printf "${COR}Compiling SystemC source: %s ...${NC}\n" "$(dir $<)$*.cpp"
	@$(CXX) -c $(dir $<)$*.cpp $(CXXFLAGS)

clean:
	@printf "Cleaning up\n"
	@rm -f *~
	@rm -f *.o
	@rm -f *.exe
	@rm -f HeMPS
	@rm -f repository*
