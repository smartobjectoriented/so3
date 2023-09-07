#include "py/mpconfig.h"
#include "py/objint.h"
#include "py/objstr.h"
#include "py/emitglue.h"
#include "py/nativeglue.h"

#if MICROPY_LONGINT_IMPL != 0
#error "incompatible MICROPY_LONGINT_IMPL"
#endif

#if MICROPY_PY_BUILTINS_FLOAT
typedef struct _mp_obj_float_t {
    mp_obj_base_t base;
    mp_float_t value;
} mp_obj_float_t;
#endif

#if MICROPY_PY_BUILTINS_COMPLEX
typedef struct _mp_obj_complex_t {
    mp_obj_base_t base;
    mp_float_t real;
    mp_float_t imag;
} mp_obj_complex_t;
#endif

enum {
    MP_QSTR_frozentest_dot_py = MP_QSTRnumber_of,
    MP_QSTR_uPy,
    MP_QSTR_i,
};

const qstr_hash_t mp_qstr_frozen_const_hashes[] = {
    254,
    249,
    204,
};

const qstr_len_t mp_qstr_frozen_const_lengths[] = {
    13,
    3,
    1,
};

extern const qstr_pool_t mp_qstr_const_pool;
const qstr_pool_t mp_qstr_frozen_const_pool = {
    &mp_qstr_const_pool, // previous pool
    MP_QSTRnumber_of, // previous pool size
    3, // allocated entries
    3, // used entries
    (qstr_hash_t *)mp_qstr_frozen_const_hashes,
    (qstr_len_t *)mp_qstr_frozen_const_lengths,
    {
        "frozentest.py",
        "uPy",
        "i",
    },
};

////////////////////////////////////////////////////////////////////////////////
// frozen module frozentest
// - original source file: ../../tests/frozen/frozentest.mpy
// - frozen file name: frozentest.py
// - .mpy header: 4d:06:02:1f

// frozen bytecode for file frozentest.py, scope frozentest__lt_module_gt_
static const byte fun_data_frozentest__lt_module_gt_[70] = {
    0x10,0x0e, // prelude
    0x01, // names: <module>
    0x27,0x27,0x27,0x27,0x2a,0x26, // code info
    0x11,0x02, // LOAD_NAME 'print'
    0x10,0x03, // LOAD_CONST_STRING 'uPy'
    0x34,0x01, // CALL_FUNCTION 1
    0x59, // POP_TOP
    0x11,0x02, // LOAD_NAME 'print'
    0x23,0x00, // LOAD_CONST_OBJ 0
    0x34,0x01, // CALL_FUNCTION 1
    0x59, // POP_TOP
    0x11,0x02, // LOAD_NAME 'print'
    0x23,0x01, // LOAD_CONST_OBJ 1
    0x34,0x01, // CALL_FUNCTION 1
    0x59, // POP_TOP
    0x11,0x02, // LOAD_NAME 'print'
    0x23,0x02, // LOAD_CONST_OBJ 2
    0x34,0x01, // CALL_FUNCTION 1
    0x59, // POP_TOP
    0x11,0x02, // LOAD_NAME 'print'
    0x22,0xba,0xef,0x9a,0x15, // LOAD_CONST_SMALL_INT 123456789
    0x34,0x01, // CALL_FUNCTION 1
    0x59, // POP_TOP
    0x80, // LOAD_CONST_SMALL_INT 0
    0x42,0x4c, // JUMP 12
    0x57, // DUP_TOP
    0x16,0x04, // STORE_NAME 'i'
    0x11,0x02, // LOAD_NAME 'print'
    0x11,0x04, // LOAD_NAME 'i'
    0x34,0x01, // CALL_FUNCTION 1
    0x59, // POP_TOP
    0x81, // LOAD_CONST_SMALL_INT 1
    0xe5, // BINARY_OP 14 __iadd__
    0x57, // DUP_TOP
    0x84, // LOAD_CONST_SMALL_INT 4
    0xd7, // BINARY_OP 0 __lt__
    0x43,0x2f, // POP_JUMP_IF_TRUE -17
    0x59, // POP_TOP
    0x51, // LOAD_CONST_NONE
    0x63, // RETURN_VALUE
};
static const mp_raw_code_t raw_code_frozentest__lt_module_gt_ = {
    .kind = MP_CODE_BYTECODE,
    .scope_flags = 0x00,
    .n_pos_args = 0,
    .fun_data = fun_data_frozentest__lt_module_gt_,
    #if MICROPY_PERSISTENT_CODE_SAVE || MICROPY_DEBUG_PRINTERS
    .fun_data_len = 70,
    #endif
    .children = NULL,
    #if MICROPY_PERSISTENT_CODE_SAVE
    .n_children = 0,
    #if MICROPY_PY_SYS_SETTRACE
    .prelude = {
        .n_state = 3,
        .n_exc_stack = 0,
        .scope_flags = 0,
        .n_pos_args = 0,
        .n_kwonly_args = 0,
        .n_def_pos_args = 0,
        .qstr_block_name_idx = 1,
        .line_info = fun_data_frozentest__lt_module_gt_ + 3,
        .line_info_top = fun_data_frozentest__lt_module_gt_ + 9,
        .opcodes = fun_data_frozentest__lt_module_gt_ + 9,
    },
    .line_of_definition = 0,
    #endif
    #if MICROPY_EMIT_MACHINE_CODE
    .prelude_offset = 0,
    #endif
    #endif
    #if MICROPY_EMIT_MACHINE_CODE
    .type_sig = 0,
    #endif
};

static const qstr_short_t const_qstr_table_data_frozentest[5] = {
    MP_QSTR_frozentest_dot_py,
    MP_QSTR__lt_module_gt_,
    MP_QSTR_print,
    MP_QSTR_uPy,
    MP_QSTR_i,
};

// constants
static const mp_obj_str_t const_obj_frozentest_0 = {{&mp_type_str}, 246, 34, (const byte*)"\x61\x20\x6c\x6f\x6e\x67\x20\x73\x74\x72\x69\x6e\x67\x20\x74\x68\x61\x74\x20\x69\x73\x20\x6e\x6f\x74\x20\x69\x6e\x74\x65\x72\x6e\x65\x64"};
static const mp_obj_str_t const_obj_frozentest_1 = {{&mp_type_str}, 200, 38, (const byte*)"\x61\x20\x73\x74\x72\x69\x6e\x67\x20\x74\x68\x61\x74\x20\x68\x61\x73\x20\x75\x6e\x69\x63\x6f\x64\x65\x20\xce\xb1\xce\xb2\xce\xb3\x20\x63\x68\x61\x72\x73"};
static const mp_obj_str_t const_obj_frozentest_2 = {{&mp_type_bytes}, 57, 11, (const byte*)"\x62\x79\x74\x65\x73\x20\x31\x32\x33\x34\x01"};

// constant table
static const mp_rom_obj_t const_obj_table_data_frozentest[3] = {
    MP_ROM_PTR(&const_obj_frozentest_0),
    MP_ROM_PTR(&const_obj_frozentest_1),
    MP_ROM_PTR(&const_obj_frozentest_2),
};

static const mp_frozen_module_t frozen_module_frozentest = {
    .constants = {
        .qstr_table = (qstr_short_t *)&const_qstr_table_data_frozentest,
        .obj_table = (mp_obj_t *)&const_obj_table_data_frozentest,
    },
    .rc = &raw_code_frozentest__lt_module_gt_,
};

////////////////////////////////////////////////////////////////////////////////
// collection of all frozen modules

const char mp_frozen_names[] = {
    #ifdef MP_FROZEN_STR_NAMES
    MP_FROZEN_STR_NAMES
    #endif
    "frozentest.py\0"
    "\0"
};

const mp_frozen_module_t *const mp_frozen_mpy_content[] = {
    &frozen_module_frozentest,
};

#ifdef MICROPY_FROZEN_LIST_ITEM
MICROPY_FROZEN_LIST_ITEM("frozentest", "frozentest.py")
#endif

/*
byte sizes:
qstr content: 3 unique, 26 bytes
bc content: 70
const str content: 83
const int content: 0
const obj content: 48
const table qstr content: 0 entries, 0 bytes
const table ptr content: 3 entries, 12 bytes
raw code content: 1 * 4 = 16
mp_frozen_mpy_names_content: 15
mp_frozen_mpy_content_size: 4
total: 274
*/
