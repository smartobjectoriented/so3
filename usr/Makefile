# GNU Makefile for building user programs to run on top of SO3
#
# Modif. 2015
#
# First setup the building environment


LIBC_DIR = $(PWD)/libc

-include Makefile.common

LDFLAGS      = -T libc/arm.lds -N -warn-common -warn-constructors -warn-multiple-gp -L $(LIBGCC_PATH) -lgcc 


# Our source apps
-include src/Makefile

USR_DIR = $(PWD)
export USR_DIR

CFLAGS += -I$(LIBC_DIR)/include

.PHONY: all clean

# make executes the first rule it finds in the Makefiles, unless it is specified
# on the comand line
all: $(patsubst %,out/%.elf,$(TARGETS))

.SECONDARY:

out/sh.elf:  src/sh.o $(LIBC_DIR)/$(NLIB)  
	$(LD) -o $@ libc/crt0.o libc/crt1.o $< -L$(LIBC_DIR) -lso3 $(LDFLAGS) -L$(LIBC_DIR)/  -lso3
	
src/%.o: src/%.c 
	$(CC) $(CFLAGS) -c $< -o $@
	
# In the following linker options, -lso3 appears twice because of cycle references
out/%.elf: src/%.o $(LIBC_DIR)/$(NLIB)  
	$(LD) -o $@ libc/crt0.o libc/crt1.o $< -L$(LIBC_DIR) -lso3 $(LDFLAGS) -L$(LIBC_DIR)/  -lso3

$(LIBC_DIR)/$(NLIB):
	@echo Checking libc...
	mkdir -p out
	make -C libc all
	
# Generic targets
clean: 
	find . -name "*.o" | xargs rm
	find . -name "*.a" | xargs rm
	rm out/*.elf
	
	
