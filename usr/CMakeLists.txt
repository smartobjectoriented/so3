
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

if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")

add_link_options(-T ${CMAKE_SOURCE_DIR}/lib/libc/arm.lds -N -warn-common 
	-warn-constructors -warn-multiple-gp -L ${libgcc_path} -lgcc
)

elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")

add_link_options(-T ${CMAKE_SOURCE_DIR}/lib/libc/aarch64.lds -N -warn-common 
	-warn-constructors -warn-multiple-gp -L ${libgcc_path} -lgcc
)

endif()

# All related libraries used by the linker
add_subdirectory(lib)

# The main applications
add_subdirectory(src)

