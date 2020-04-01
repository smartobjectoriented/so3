# GNU Makefile for building user programs to run on top of SO3
#
# Modif. 2015
#
# First setup the building environment


LIBC_DIR = $(PWD)/libc
export LIBC_DIR

LVGL_DIR = $(PWD)/lvgl
LVGL_LIB = liblvgl.a
export LVGL_DIR LVGL_LIB

-include Makefile.common

LDFLAGS = -T libc/arm.lds -N -warn-common -warn-constructors -warn-multiple-gp -L $(LIBGCC_PATH) -lgcc

# Our source apps
-include src/Makefile

USR_DIR = $(PWD)
export USR_DIR

CFLAGS += -I$(LIBC_DIR)/include -I$(LVGL_DIR)


.PHONY: all clean check_out_dir

# make executes the first rule it finds in the Makefiles, unless it is specified
# on the comand line
all: check_out_dir $(patsubst %,out/%.elf,$(TARGETS))

check_out_dir:
	@mkdir -p out
	make -C libc all

# Avoid make to use pre-defined rules and keep intermediate productions
.SECONDARY: $(patsubst %,src/%.o,$(TARGETS))

out/sh.elf: src/sh.o $(LIBC_DIR)/$(NLIB)
	$(LD) -o $@ libc/crt0.o libc/crt1.o $< -L$(LIBC_DIR) -lso3 $(LDFLAGS) -L$(LIBC_DIR)/ -lso3

src/%.o: src/%.c 
	$(CC) $(CFLAGS) -c $< -o $@

# In the following linker options, -lso3 appears twice because of cycle references
out/%.elf: src/%.o $(LIBC_DIR)/$(NLIB) $(LVGL_DIR)/$(LVGL_LIB)
	$(LD) -o $@ libc/crt0.o libc/crt1.o $< -L$(LIBC_DIR) -lso3 -L$(LVGL_DIR) -llvgl $(LDFLAGS) -L$(LIBC_DIR)/ -lso3

$(LIBC_DIR)/$(NLIB):
	@echo Checking libc...
	make -C libc all

# Build LittlevGL as library.
$(LVGL_DIR)/$(LVGL_LIB):
	make -C lvgl all

# Generic targets
clean:
	find . -name "*.o" | xargs -r rm
	find . -name "*.a" | xargs -r rm
	find out -name "*.elf" | xargs -r rm
