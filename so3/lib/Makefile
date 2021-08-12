lib-y += lprintk.o
lib-y += string.o string_helpers.o
lib-y += ctype.o
lib-y += vsprintf.o printk.o

# These files are defined for ARM arch only currently.
# RISC-V has theses defined in arch/riscv64/lib 
ifndef CONFIG_ARCH_RISCV64
    lib-y += findbit.o strchr.o strtod.o
endif