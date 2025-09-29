# aarch64-musl.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-musl-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-musl-g++)

# Optional: force static linking
set(CMAKE_EXE_LINKER_FLAGS "-static -fno-rtti -Os -s")
