
if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")

file(CREATE_LINK "${CMAKE_CURRENT_SOURCE_DIR}/include/asm-aarch32" "${CMAKE_CURRENT_SOURCE_DIR}/include/asm" SYMBOLIC)

else()

file(CREATE_LINK "${CMAKE_CURRENT_SOURCE_DIR}/include/asm-aarch64" "${CMAKE_CURRENT_SOURCE_DIR}/include/asm" SYMBOLIC)

endif()


include_directories(
        include
)

add_library(c STATIC 
	crt0.S crt1.c 
	pthread.c inet.c eabi_compat.c malloc.c 
	shgetc.c intscan.c floatscan.c mutex.c sbrk.c)

add_subdirectory(string)
add_subdirectory(stdio)
add_subdirectory(fcntl)
add_subdirectory(misc)
add_subdirectory(errno)
add_subdirectory(multibyte)
add_subdirectory(ctype)
add_subdirectory(thread)
add_subdirectory(stdlib)
add_subdirectory(dirent)
add_subdirectory(unistd)
add_subdirectory(env)
add_subdirectory(malloc)
add_subdirectory(math)
add_subdirectory(exit)
add_subdirectory(process)
add_subdirectory(signal)
add_subdirectory(time)
add_subdirectory(prng)
add_subdirectory(mman)
add_subdirectory(network)
