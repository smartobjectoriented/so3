
-include $(USR_DIR)/Makefile.common

LIB = socket inet_ntoa sendto inet_pton recvfrom setsockopt

all: $(patsubst %,%.o,$(LIB))

clean:
	rm -rf *.o

	
