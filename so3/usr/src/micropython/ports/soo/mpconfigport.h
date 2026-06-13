#include <stdint.h>
#include <unistd.h>

// options to control how MicroPython is built

// Use the minimal starting configuration (disables all optional features).
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)

#define MICROPY_PY_ARRAY (1)
#define MICROPY_PY_COLLECTIONS (1)
#define MICROPY_PY_STRUCT (1)

// You can disable the built-in MicroPython compiler by setting the following
// config option to 0.  If you do this then you won't get a REPL prompt, but you
// will still be able to execute pre-compiled scripts, compiled with mpy-cross.
#define MICROPY_ENABLE_COMPILER (1)

#define MICROPY_QSTR_EXTRA_POOL mp_qstr_frozen_const_pool
#define MICROPY_ENABLE_GC (1)
#define MICROPY_HELPER_REPL (1)
#define MICROPY_MODULE_FROZEN_MPY (1)
#define MICROPY_ENABLE_EXTERNAL_IMPORT (1)
#define MICROPY_READER_POSIX (1)

#define MICROPY_ALLOC_PATH_MAX (256)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT (16)

// type definitions for the specific machine

typedef intptr_t mp_int_t; // must be pointer size
typedef uintptr_t mp_uint_t; // must be pointer size
typedef long mp_off_t;

// We need to provide a declaration/definition of alloca()
#include <alloca.h>

#define MICROPY_HW_BOARD_NAME "minimal"
#define MICROPY_HW_MCU_NAME "unknown-cpu"

#define MICROPY_MIN_USE_STDOUT (1)
#define MICROPY_HEAP_SIZE (25600) // heap size 25 kilobytes

#ifdef __thumb__
#define MICROPY_MIN_USE_CORTEX_CPU (1)
#define MICROPY_MIN_USE_STM32_MCU (1)
#define MICROPY_HEAP_SIZE (2048) // heap size 2 kilobytes
#endif

#define MP_STATE_PORT MP_STATE_VM
