 
-include $(USR_DIR)/Makefile.common

LIB = clearenv getenv putenv setenv unsetenv __environ

all: $(patsubst %,%.o,$(LIB))


clean:
	rm -rf *.o
