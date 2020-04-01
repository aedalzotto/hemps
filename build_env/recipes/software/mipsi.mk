RED  =\033[0;31m
NC   =\033[0m # No Color

KERNEL_PKG_TGT    = kernel_pkg.o
KERNEL_PKG_SRC    = ../../include/kernel_pkg.c

CFLAGS            = -O2 -Wall -fms-extensions -c -s -std=c99 -G 0
GCC_MIPS          = mips-elf-gcc $(CFLAGS)
AS_MIPS           = mips-elf-as
LD_MIPS           = mips-elf-ld
DUMP_MIPS         = mips-elf-objdump
COPY_MIPS         = mips-elf-objcopy -I elf32-bigmips -O binary

BOOT_MASTER       = boot_master
BOOT_MASTER_SRC   = ../kernel/master/plasma/boot.S
KERNEL_MASTER 	  = kernel_master
KERNEL_MASTER_SRC = ../kernel/master/kernel_master.c

BOOT_SLAVE        = boot_slave
BOOT_SLAVE_SRC	  = ../kernel/slave/plasma/boot.S
KERNEL_SLAVE 	  = kernel_slave
KERNEL_SLAVE_SRC  = ../kernel/slave/kernel_slave.c 

#https://www.gnu.org/software/make/manual/make.html#Automatic-Variables

MODULES_DIR = ../modules
MODULES_DEST = .
MODULES_NAMES = utils packet applications pending_service reclustering new_task communication processors task_control task_location task_migration task_scheduler resource_manager
MODULES_SRC = $(addsuffix .c, $(addprefix $(MODULES_DIR)/, $(MODULES_NAMES)))
MODULES_TGT = $(addprefix $(MODULES_DEST)/,$(notdir $(MODULES_SRC:.c=.o)))

CPU_DIR = cpu
CPU_SRC = ../$(CPU_DIR)/plasma.c
CPU_OBJ = plasma.o

KERNEL_MASTER_MODULES = utils packet applications reclustering new_task communication processors resource_manager
KERNEL_MASTER_TGT     = $(addsuffix .o, $(addprefix $(MODULES_DEST)/, $(KERNEL_MASTER_MODULES)))

KERNEL_SLAVE_MODULES = utils packet pending_service communication task_control task_location task_migration task_scheduler
KERNEL_SLAVE_TGT 	 = $(addsuffix .o, $(addprefix $(MODULES_DEST)/, $(KERNEL_SLAVE_MODULES)))

default: $(KERNEL_MASTER).txt $(KERNEL_SLAVE).txt 

$(KERNEL_PKG_TGT): $(KERNEL_PKG_SRC)
	@printf "${RED}Compiling MIPSI Kernel Package: %s ...${NC}\n" "$<"
	@$(GCC_MIPS) -c $< -o $@

$(MODULES_DEST)/%.o: $(MODULES_DIR)/%.c
	@printf "${RED}Compiling MIPSI Kernel %s ...${NC}\n" "$<"
	@$(GCC_MIPS) -c $< -o $@

$(CPU_OBJ) : $(CPU_SRC)
	@printf "${RED}Compiling MIPSI Kernel CPU code: %s ...${NC}\n" "$*.c"
	@$(GCC_MIPS) $(CFLAGS) $< -o $@ 

$(KERNEL_MASTER).txt: $(KERNEL_PKG_TGT) $(MODULES_TGT) $(KERNEL_MASTER_SRC) $(BOOT_MASTER_SRC) $(CPU_OBJ)
	@printf "${RED}Compiling MIPSI Kernel Master: %s ...${NC}\n" "$(KERNEL_MASTER).c" 
	@$(AS_MIPS) --defsym sp_addr=$(MEM_SP_INIT) -o $(BOOT_MASTER).o $(BOOT_MASTER_SRC)
	@$(GCC_MIPS) -DHOP_NUMBER=1 -Dload -o $(KERNEL_MASTER).o $(KERNEL_MASTER_SRC) -D IS_MASTER
	@$(LD_MIPS) --section-start=".init"=0 -Map $(KERNEL_MASTER).map -s -N -o $(KERNEL_MASTER).elf $(BOOT_MASTER).o $(KERNEL_MASTER).o $(KERNEL_MASTER_TGT) $(KERNEL_PKG_TGT) $(CPU_OBJ)
	@$(LD_MIPS) --section-start=".init"=0 -Map $(KERNEL_MASTER)_debug.map -o $(KERNEL_MASTER)_debug.elf $(BOOT_MASTER).o $(KERNEL_MASTER).o $(KERNEL_MASTER_TGT) $(KERNEL_PKG_TGT) $(CPU_OBJ)
	@$(DUMP_MIPS) -S $(KERNEL_MASTER)_debug.elf > $(KERNEL_MASTER).lst
	@$(COPY_MIPS) -R .MIPS.abiflags -R .reginfo $(KERNEL_MASTER).elf $(KERNEL_MASTER).bin
	@hexdump -v -e '1/1 "%02x" 1/1 "%02x" 1/1 "%02x" 1/1 "%02x" "\n"' $(KERNEL_MASTER).bin > $(KERNEL_MASTER).txt

$(KERNEL_SLAVE).txt: $(KERNEL_PKG_TGT) $(MODULES_TGT) $(KERNEL_SLAVE_SRC) $(BOOT_SLAVE_SRC) $(CPU_OBJ)
	@printf "${RED}Compiling MIPSI Kernel Slave: %s ...${NC}\n" "$(KERNEL_SLAVE).c"
	@$(AS_MIPS) --defsym sp_addr=$(PAGE_SP_INIT) -o $(BOOT_SLAVE).o $(BOOT_SLAVE_SRC)
	@$(GCC_MIPS) -o $(KERNEL_SLAVE).o $(KERNEL_SLAVE_SRC) -D PLASMA
	@$(LD_MIPS) --section-start=".init"=0 -Map $(KERNEL_SLAVE).map -s -N -o $(KERNEL_SLAVE).elf $(BOOT_SLAVE).o $(KERNEL_SLAVE).o $(KERNEL_SLAVE_TGT) $(KERNEL_PKG_TGT) $(CPU_OBJ)
	@$(LD_MIPS) --section-start=".init"=0 -Map $(KERNEL_SLAVE)_debug.map -o $(KERNEL_SLAVE)_debug.elf $(BOOT_SLAVE).o $(KERNEL_SLAVE).o $(KERNEL_SLAVE_TGT) $(KERNEL_PKG_TGT) $(CPU_OBJ)
	@$(DUMP_MIPS) -S $(KERNEL_SLAVE)_debug.elf > $(KERNEL_SLAVE).lst
	@$(COPY_MIPS) -R .MIPS.abiflags -R .reginfo $(KERNEL_SLAVE).elf $(KERNEL_SLAVE).bin
	@hexdump -v -e '1/1 "%02x" 1/1 "%02x" 1/1 "%02x" 1/1 "%02x" "\n"' $(KERNEL_SLAVE).bin > $(KERNEL_SLAVE).txt

clean:
	@printf "Cleaning up\n"
	@rm -rf modules/*.o
	@rm -rf *.o
	@rm -rf *.bin
	@rm -rf *.map
	@rm -rf *.lst
	@rm -rf *.txt
	@rm -rf *.elf

