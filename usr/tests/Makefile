# GNU Makefile for building user programs to run on top of SO3
#
# Modif. 2015
#
# First setup the building environment


LIBC_DIR = $(PWD)/../libc

-include Makefile.common

LDFLAGS      = -T $(LIBC_DIR)/arm.lds -N -warn-common -warn-constructors -warn-multiple-gp -L $(LIBGCC_PATH) -lgcc 


# Our source apps
-include src/Makefile

#USR_DIR = $(PWD)
#export USR_DIR

CFLAGS += -I$(LIBC_DIR)/include

.PHONY: all clean check_out_dir

# make executes the first rule it finds in the Makefiles, unless it is specified
# on the comand line
all: check_out_dir $(patsubst %,out/%.elf,$(TARGETS))

check_out_dir:
	@mkdir -p out
	make -C $(LIBC_DIR) all

# Avoid make to use pre-defined rules and keep intermediate productions
.SECONDARY: $(patsubst %,src/%.o,$(TARGETS))

src/%.o: src/%.c 
	$(CC) $(CFLAGS) -c $< -o $@

# In the following linker options, -lso3 appears twice because of cycle references
out/%.elf: src/%.o $(LIBC_DIR)/$(NLIB) 
	$(LD) -o $@ $(LIBC_DIR)/crt0.o $(LIBC_DIR)/crt1.o $< -L$(LIBC_DIR) -lso3 $(LDFLAGS) -L$(LIBC_DIR)/  -lso3

$(LIBC_DIR)/$(NLIB):
	@echo Checking libc...
	make -C $(LIBC_DIR) all

# Generic targets
clean: 
	find . -name "*.o" | xargs rm
	find . -name "*.a" | xargs rm
	rm out/*.elf

