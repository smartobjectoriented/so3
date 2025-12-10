
# CMake for building all SO3 usr applications
# Copyright (c) 2021 Daniel Rossier, REDS Institute - HEIG-VD

cmake_minimum_required(VERSION 3.16)

project(so3-usr LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MICROPYTHON "Support for micro-python apps" OFF)

# Where the header files are located
include_directories(  
     src
     lib
     lib/lvgl
     lib/lvgl/src
     lib/slv
     lib/logs/include
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

add_link_options(-N -Wl,--warn-common
)

elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")

add_link_options(-N -Wl,--warn-common
)

endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	add_compile_options(-ggdb)
endif()

# All related libraries used by the linker
add_subdirectory(lib)

# The main applications
add_subdirectory(src)

