 
-include $(USR_DIR)/Makefile.common

LIB =  printf fprintf sprintf vsprintf vsnprintf snprintf stdin stdout stderr getchar putchar vfprintf putc getc scanf vscanf vfscanf 
LIB += fopen fread fwrite fclose fgetc fputc fputs fgets fseek ftell
LIB += fgets puts gets fileno perror ofl_add
LIB += __stdio_exit __stdio_write __stdout_write __stdio_close __stdio_read __stdio_seek __uflow __lockfile __towrite __toread
LIB += __fmodeflags __fdopen
LIB += ofl __overflow fflush

all: $(patsubst %,%.o,$(LIB))


clean:
	rm -rf *.o
