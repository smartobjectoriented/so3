
-include $(USR_DIR)/Makefile.common

LIB = execl execle execve execv execvp execlp ptrace fork waitpid

all: $(patsubst %,%.o,$(LIB))

clean:
	rm -rf *.o

	
