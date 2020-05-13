
-include $(USR_DIR)/Makefile.common

LIB = socket inet_ntoa sendto inet_pton inet_ntop recvfrom setsockopt

all: $(patsubst %,%.o,$(LIB))

clean:
	rm -rf *.o

	
