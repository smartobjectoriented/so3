 
-include $(USR_DIR)/Makefile.common

LIB =  printf fprintf sprintf vsprintf vsnprintf stdin stdout stderr getchar putchar fgetc fputc fwrite vfprintf putc getc scanf vscanf vfscanf
LIB += fgets puts gets fileno
LIB += __stdio_exit __stdio_write __stdout_write __stdio_close __stdio_read __stdio_seek __uflow __lockfile __towrite __toread
LIB += ofl __overflow fflush

all: $(patsubst %,%.o,$(LIB))


clean:
	rm -rf *.o
