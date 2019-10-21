GREEN  =\033[0;32m
NC   =\033[0m # No Color

INCLUDE           = ../../software/include

CFLAGS            = -O2 -Wall -std=c99 -s
GCC		          = riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32
AS	          	  = riscv64-unknown-elf-as -march=rv32im -mabi=ilp32
LD		          = riscv64-unknown-elf-ld -melf32lriscv
DUMP	          = riscv64-unknown-elf-objdump
COPY	          = riscv64-unknown-elf-objcopy -O binary
BOOT_TASK         = boot_task
BOOT_TASK_SRC     = ../../software/boot_task/riscv/boot_task.asm

TASK_SRC = $(wildcard *.c)
TASK_OBJ = $(TASK_SRC:.c=.o)

default: $(BOOT_TASK).o $(TASK_OBJ) 

boot_task.o: $(BOOT_TASK_SRC)
	@printf "${GREEN}Compiling boot task: %s ...${NC}\n" "$*.c"
	@$(AS) --defsym sp_addr=$(PAGE_SP_INIT) -o $@ $^

#[$*] - only the filename - #https://www.gnu.org/software/make/manual/make.html#Automatic-Variables
$(TASK_OBJ): $(TASK_SRC) 
	@printf "${GREEN}Compiling task: %s ...${NC}\n" "$*.c"
	@$(GCC) $(CFLAGS) -c $*.c -o $*.o --include id_tasks.h -I $(INCLUDE)
	@$(LD) --section-start=".init"=0 -Map $*.map -s -N -o $*.elf $(BOOT_TASK).o $*.o
	@$(LD) --section-start=".init"=0 -Map $*_debug.map -o $*_debug.elf $(BOOT_TASK).o $*.o
	@$(DUMP) -S $*_debug.elf > $*.lst
	@$(COPY) $*.elf $*.bin
	@hexdump -v -e '1/4 "%08x" "\n"' $*.bin > $*.txt

clean:
	@printf "Cleaning up\n"
	@rm -rf *.o
	@rm -rf *.bin
	@rm -rf *.map
	@rm -rf *.lst
	@rm -rf *.txt
	@rm -rf *.dump
