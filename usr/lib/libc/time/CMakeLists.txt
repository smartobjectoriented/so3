
target_sources(c 
	PRIVATE
		nanosleep.c
		gettimeofday.c
		time.c
		clock_gettime.c
		ctime.c
		localtime.c
		asctime.c
		localtime_r.c
		strftime.c
		__asctime.c
		__year_to_secs.c
		__secs_to_tm.c
		__month_to_secs.c
		__tm_to_secs.c
)
