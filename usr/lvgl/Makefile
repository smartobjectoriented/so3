#
# Build LittlevGL as a library.
#
# Inspiration:
#   - https://github.com/littlevgl/lv_port_linux_frame_buffer/blob/master/Makefile
#

include ../Makefile.common
include lvgl/lvgl.mk

# CSRCS is defined in each of the *.mk files and contains all the C files that
# need to be compiled.
COBJS = $(CSRCS:.c=.o)


all: $(LVGL_LIB)

# Compile all C files.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Create the library.
$(LVGL_LIB): $(COBJS)
	$(AR) rv $@ *.o

clean:
	rm $(LVGL_LIB) $(COBJS)
