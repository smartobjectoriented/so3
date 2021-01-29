
-include $(USR_DIR)/Makefile.common

LIB = nanosleep  gettimeofday time clock_gettime ctime localtime asctime localtime_r __asctime __year_to_secs __secs_to_tm __month_to_secs

all: $(patsubst %,%.o,$(LIB))

clean:
	rm -rf *.o

	
