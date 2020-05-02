 
-include $(USR_DIR)/Makefile.common

#LIB = __libc_start_main
LIB = clearenv getenv putenv setenv unsetenv __environ

all: $(patsubst %,%.o,$(LIB))


clean:
	rm -rf *.o
