
-include $(USR_DIR)/Makefile.common

LIB = socket inet_ntoa send sendto inet_pton inet_ntop recv recvfrom setsockopt htonl htons bind listen accept connect

all: $(patsubst %,%.o,$(LIB))

clean:
	rm -rf *.o

	
