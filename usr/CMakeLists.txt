
# CMake for building all SO3 usr applications
# Copyright (c) 2021 Daniel Rossier, REDS Institute - HEIG-VD

cmake_minimum_required(VERSION 3.16)

project(so3-usr LANGUAGES C ASM)

# Detect the correct compiler runtime library and add it
# to 'LIBCXX_LIBRARIES'. back to gcc_s if available.

# Where the header files are located
include_directories(  
     src
     lib
     lib/libc/include
     lib/lvgl
     lib/lvgl/src
)
 
set(CMAKE_C_FLAGS "-Wall -O0 -std=c99  -D_GNU_SOURCE -nostdlib -O0 -pipe -Wall  -D__ARM__ -marm  \
	-mno-thumb-interwork -march=armv5 -g -ffreestanding -fno-common")

set(CMAKE_ASM_FLAGS_DEBUG "-D__ASSEMBLY__")
set(CMAKE_LINKER "arm-linux-gnueabihf-ld")

set(libgcc_cmd ${CMAKE_C_COMPILER} -print-libgcc-file-name)

execute_process(
	COMMAND ${libgcc_cmd} 
	OUTPUT_VARIABLE libgcc_name
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
	COMMAND dirname ${libgcc_name}
	OUTPUT_VARIABLE libgcc_path
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_link_options(-T ${CMAKE_SOURCE_DIR}/lib/libc/arm.lds -N -warn-common 
	-warn-constructors -warn-multiple-gp -L ${libgcc_path} -lgcc
)

# All related libraries used by the linker
add_subdirectory(lib)

# The main applications
add_subdirectory(src)

