
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")

target_sources(c 
	PRIVATE
		__lock.c
		atomics.S
		thrd_sleep.c
)

else()

target_sources(c 
	PRIVATE
		__lock.c
		thrd_sleep.c
)

endif()
