/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * QAPI/QMP schema introspection
 *
 * Copyright (C) 2015-2018 Red Hat, Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "test-qapi-introspect.h"

const QLitObject test_qmp_schema_qlit = QLIT_QLIST(((QLitObject[]) {
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("0"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("user_def_cmd0"), },
        { "ret-type", QLIT_QSTR("0"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("1"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("user_def_cmd"), },
        { "ret-type", QLIT_QSTR("1"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("2"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("user_def_cmd1"), },
        { "ret-type", QLIT_QSTR("1"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("3"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("user_def_cmd2"), },
        { "ret-type", QLIT_QSTR("4"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("1"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("cmd-success-response"), },
        { "ret-type", QLIT_QSTR("1"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("5"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("guest-get-time"), },
        { "ret-type", QLIT_QSTR("int"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("6"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("guest-sync"), },
        { "ret-type", QLIT_QSTR("any"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("7"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("boxed-struct"), },
        { "ret-type", QLIT_QSTR("1"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("8"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("boxed-union"), },
        { "ret-type", QLIT_QSTR("1"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "allow-oob", QLIT_QBOOL(true), },
        { "arg-type", QLIT_QSTR("1"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("test-flags-command"), },
        { "ret-type", QLIT_QSTR("1"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("1"), },
        { "meta-type", QLIT_QSTR("event"), },
        { "name", QLIT_QSTR("EVENT_A"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("1"), },
        { "meta-type", QLIT_QSTR("event"), },
        { "name", QLIT_QSTR("EVENT_B"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("9"), },
        { "meta-type", QLIT_QSTR("event"), },
        { "name", QLIT_QSTR("EVENT_C"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("10"), },
        { "meta-type", QLIT_QSTR("event"), },
        { "name", QLIT_QSTR("EVENT_D"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("7"), },
        { "meta-type", QLIT_QSTR("event"), },
        { "name", QLIT_QSTR("EVENT_E"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("11"), },
        { "meta-type", QLIT_QSTR("event"), },
        { "name", QLIT_QSTR("EVENT_F"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("12"), },
        { "meta-type", QLIT_QSTR("event"), },
        { "name", QLIT_QSTR("__ORG.QEMU_X-EVENT"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("13"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("__org.qemu_x-command"), },
        { "ret-type", QLIT_QSTR("14"), },
        {}
    })),
#if defined(TEST_IF_UNION)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("15"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("TestIfUnionCmd"), },
        { "ret-type", QLIT_QSTR("1"), },
        {}
    })),
#endif /* defined(TEST_IF_UNION) */
#if defined(TEST_IF_ALT)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("16"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("TestIfAlternateCmd"), },
        { "ret-type", QLIT_QSTR("1"), },
        {}
    })),
#endif /* defined(TEST_IF_ALT) */
#if defined(TEST_IF_CMD)
#if defined(TEST_IF_STRUCT)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("17"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("TestIfCmd"), },
        { "ret-type", QLIT_QSTR("18"), },
        {}
    })),
#endif /* defined(TEST_IF_STRUCT) */
#endif /* defined(TEST_IF_CMD) */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("1"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("TestCmdReturnDefThree"), },
        { "ret-type", QLIT_QSTR("18"), },
        {}
    })),
#if defined(TEST_IF_EVT) && defined(TEST_IF_STRUCT)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("19"), },
        { "meta-type", QLIT_QSTR("event"), },
        { "name", QLIT_QSTR("TestIfEvent"), },
        {}
    })),
#endif /* defined(TEST_IF_EVT) && defined(TEST_IF_STRUCT) */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "arg-type", QLIT_QSTR("20"), },
        { "meta-type", QLIT_QSTR("command"), },
        { "name", QLIT_QSTR("test-features"), },
        { "ret-type", QLIT_QSTR("1"), },
        {}
    })),
    /* "0" = Empty2 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("0"), },
        {}
    })),
    /* "1" = q_empty */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("1"), },
        {}
    })),
    /* "2" = q_obj_user_def_cmd1-arg */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("ud1a"), },
                { "type", QLIT_QSTR("21"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("2"), },
        {}
    })),
    /* "3" = q_obj_user_def_cmd2-arg */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("ud1a"), },
                { "type", QLIT_QSTR("21"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("ud1b"), },
                { "type", QLIT_QSTR("21"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("3"), },
        {}
    })),
    /* "4" = UserDefTwo */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string0"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("dict1"), },
                { "type", QLIT_QSTR("22"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("4"), },
        {}
    })),
    /* "5" = q_obj_guest-get-time-arg */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("a"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("b"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("5"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("int"), },
        { "meta-type", QLIT_QSTR("builtin"), },
        { "name", QLIT_QSTR("int"), },
        {}
    })),
    /* "6" = q_obj_guest-sync-arg */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("arg"), },
                { "type", QLIT_QSTR("any"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("6"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("value"), },
        { "meta-type", QLIT_QSTR("builtin"), },
        { "name", QLIT_QSTR("any"), },
        {}
    })),
    /* "7" = UserDefZero */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("integer"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("7"), },
        {}
    })),
    /* "8" = UserDefListUnion */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type"), },
                { "type", QLIT_QSTR("23"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("8"), },
        { "tag", QLIT_QSTR("type"), },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("integer"), },
                { "type", QLIT_QSTR("24"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("s8"), },
                { "type", QLIT_QSTR("25"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("s16"), },
                { "type", QLIT_QSTR("26"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("s32"), },
                { "type", QLIT_QSTR("27"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("s64"), },
                { "type", QLIT_QSTR("28"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("u8"), },
                { "type", QLIT_QSTR("29"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("u16"), },
                { "type", QLIT_QSTR("30"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("u32"), },
                { "type", QLIT_QSTR("31"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("u64"), },
                { "type", QLIT_QSTR("32"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("number"), },
                { "type", QLIT_QSTR("33"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("boolean"), },
                { "type", QLIT_QSTR("34"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("string"), },
                { "type", QLIT_QSTR("35"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("sizes"), },
                { "type", QLIT_QSTR("36"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("any"), },
                { "type", QLIT_QSTR("37"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("user"), },
                { "type", QLIT_QSTR("38"), },
                {}
            })),
            {}
        })), },
        {}
    })),
    /* "9" = q_obj_EVENT_C-arg */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("a"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("b"), },
                { "type", QLIT_QSTR("21"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("c"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("9"), },
        {}
    })),
    /* "10" = q_obj_EVENT_D-arg */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("a"), },
                { "type", QLIT_QSTR("39"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("b"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("c"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("enum3"), },
                { "type", QLIT_QSTR("40"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("10"), },
        {}
    })),
    /* "11" = UserDefAlternate */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("41"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("40"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("null"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("alternate"), },
        { "name", QLIT_QSTR("11"), },
        {}
    })),
    /* "12" = __org.qemu_x-Struct */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("__org.qemu_x-member1"), },
                { "type", QLIT_QSTR("42"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("__org.qemu_x-member2"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("wchar-t"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("12"), },
        {}
    })),
    /* "13" = q_obj___org.qemu_x-command-arg */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("a"), },
                { "type", QLIT_QSTR("[42]"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("b"), },
                { "type", QLIT_QSTR("[12]"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("c"), },
                { "type", QLIT_QSTR("43"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("d"), },
                { "type", QLIT_QSTR("44"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("13"), },
        {}
    })),
    /* "14" = __org.qemu_x-Union1 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type"), },
                { "type", QLIT_QSTR("45"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("14"), },
        { "tag", QLIT_QSTR("type"), },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("__org.qemu_x-branch"), },
                { "type", QLIT_QSTR("46"), },
                {}
            })),
            {}
        })), },
        {}
    })),
    /* "15" = q_obj_TestIfUnionCmd-arg */
#if defined(TEST_IF_UNION)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("union_cmd_arg"), },
                { "type", QLIT_QSTR("47"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("15"), },
        {}
    })),
#endif /* defined(TEST_IF_UNION) */
    /* "16" = q_obj_TestIfAlternateCmd-arg */
#if defined(TEST_IF_ALT)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("alt_cmd_arg"), },
                { "type", QLIT_QSTR("48"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("16"), },
        {}
    })),
#endif /* defined(TEST_IF_ALT) */
    /* "17" = q_obj_TestIfCmd-arg */
#if defined(TEST_IF_CMD)
#if defined(TEST_IF_STRUCT)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("foo"), },
                { "type", QLIT_QSTR("49"), },
                {}
            })),
#if defined(TEST_IF_CMD_BAR)
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bar"), },
                { "type", QLIT_QSTR("50"), },
                {}
            })),
#endif /* defined(TEST_IF_CMD_BAR) */
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("17"), },
        {}
    })),
#endif /* defined(TEST_IF_STRUCT) */
#endif /* defined(TEST_IF_CMD) */
    /* "18" = UserDefThree */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string0"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("18"), },
        {}
    })),
    /* "19" = q_obj_TestIfEvent-arg */
#if defined(TEST_IF_EVT) && defined(TEST_IF_STRUCT)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("foo"), },
                { "type", QLIT_QSTR("49"), },
                {}
            })),
#if defined(TEST_IF_EVT_BAR)
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bar"), },
                { "type", QLIT_QSTR("[50]"), },
                {}
            })),
#endif /* defined(TEST_IF_EVT_BAR) */
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("19"), },
        {}
    })),
#endif /* defined(TEST_IF_EVT) && defined(TEST_IF_STRUCT) */
    /* "20" = q_obj_test-features-arg */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fs0"), },
                { "type", QLIT_QSTR("51"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fs1"), },
                { "type", QLIT_QSTR("52"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fs2"), },
                { "type", QLIT_QSTR("53"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fs3"), },
                { "type", QLIT_QSTR("54"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("fs4"), },
                { "type", QLIT_QSTR("55"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cfs1"), },
                { "type", QLIT_QSTR("56"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cfs2"), },
                { "type", QLIT_QSTR("57"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("cfs3"), },
                { "type", QLIT_QSTR("58"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("20"), },
        {}
    })),
    /* "21" = UserDefOne */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("integer"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("enum1"), },
                { "type", QLIT_QSTR("40"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("21"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("string"), },
        { "meta-type", QLIT_QSTR("builtin"), },
        { "name", QLIT_QSTR("str"), },
        {}
    })),
    /* "22" = UserDefTwoDict */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string1"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("dict2"), },
                { "type", QLIT_QSTR("59"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("dict3"), },
                { "type", QLIT_QSTR("59"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("22"), },
        {}
    })),
    /* "23" = UserDefListUnionKind */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum"), },
        { "name", QLIT_QSTR("23"), },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("integer"),
            QLIT_QSTR("s8"),
            QLIT_QSTR("s16"),
            QLIT_QSTR("s32"),
            QLIT_QSTR("s64"),
            QLIT_QSTR("u8"),
            QLIT_QSTR("u16"),
            QLIT_QSTR("u32"),
            QLIT_QSTR("u64"),
            QLIT_QSTR("number"),
            QLIT_QSTR("boolean"),
            QLIT_QSTR("string"),
            QLIT_QSTR("sizes"),
            QLIT_QSTR("any"),
            QLIT_QSTR("user"),
            {}
        })), },
        {}
    })),
    /* "24" = q_obj_intList-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[int]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("24"), },
        {}
    })),
    /* "25" = q_obj_int8List-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[int]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("25"), },
        {}
    })),
    /* "26" = q_obj_int16List-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[int]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("26"), },
        {}
    })),
    /* "27" = q_obj_int32List-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[int]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("27"), },
        {}
    })),
    /* "28" = q_obj_int64List-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[int]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("28"), },
        {}
    })),
    /* "29" = q_obj_uint8List-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[int]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("29"), },
        {}
    })),
    /* "30" = q_obj_uint16List-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[int]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("30"), },
        {}
    })),
    /* "31" = q_obj_uint32List-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[int]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("31"), },
        {}
    })),
    /* "32" = q_obj_uint64List-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[int]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("32"), },
        {}
    })),
    /* "33" = q_obj_numberList-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[number]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("33"), },
        {}
    })),
    /* "34" = q_obj_boolList-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[bool]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("34"), },
        {}
    })),
    /* "35" = q_obj_strList-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[str]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("35"), },
        {}
    })),
    /* "36" = q_obj_sizeList-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[int]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("36"), },
        {}
    })),
    /* "37" = q_obj_anyList-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[any]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("37"), },
        {}
    })),
    /* "38" = q_obj_StatusList-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("[60]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("38"), },
        {}
    })),
    /* "39" = EventStructOne */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("struct1"), },
                { "type", QLIT_QSTR("21"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("enum2"), },
                { "type", QLIT_QSTR("40"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("39"), },
        {}
    })),
    /* "40" = EnumOne */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum"), },
        { "name", QLIT_QSTR("40"), },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("value1"),
            QLIT_QSTR("value2"),
            QLIT_QSTR("value3"),
            QLIT_QSTR("value4"),
            {}
        })), },
        {}
    })),
    /* "41" = UserDefFlatUnion */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("integer"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("enum1"), },
                { "type", QLIT_QSTR("40"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("41"), },
        { "tag", QLIT_QSTR("enum1"), },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("value1"), },
                { "type", QLIT_QSTR("61"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("value2"), },
                { "type", QLIT_QSTR("62"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("value3"), },
                { "type", QLIT_QSTR("62"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("value4"), },
                { "type", QLIT_QSTR("1"), },
                {}
            })),
            {}
        })), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("null"), },
        { "meta-type", QLIT_QSTR("builtin"), },
        { "name", QLIT_QSTR("null"), },
        {}
    })),
    /* "42" = __org.qemu_x-Enum */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum"), },
        { "name", QLIT_QSTR("42"), },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("__org.qemu_x-value"),
            {}
        })), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("42"), },
        { "meta-type", QLIT_QSTR("array"), },
        { "name", QLIT_QSTR("[42]"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("12"), },
        { "meta-type", QLIT_QSTR("array"), },
        { "name", QLIT_QSTR("[12]"), },
        {}
    })),
    /* "43" = __org.qemu_x-Union2 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("__org.qemu_x-member1"), },
                { "type", QLIT_QSTR("42"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("43"), },
        { "tag", QLIT_QSTR("__org.qemu_x-member1"), },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("__org.qemu_x-value"), },
                { "type", QLIT_QSTR("63"), },
                {}
            })),
            {}
        })), },
        {}
    })),
    /* "44" = __org.qemu_x-Alt */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("64"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("alternate"), },
        { "name", QLIT_QSTR("44"), },
        {}
    })),
    /* "45" = __org.qemu_x-Union1Kind */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum"), },
        { "name", QLIT_QSTR("45"), },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("__org.qemu_x-branch"),
            {}
        })), },
        {}
    })),
    /* "46" = q_obj_str-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("46"), },
        {}
    })),
    /* "47" = TestIfUnion */
#if defined(TEST_IF_UNION) && defined(TEST_IF_STRUCT)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("type"), },
                { "type", QLIT_QSTR("65"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("47"), },
        { "tag", QLIT_QSTR("type"), },
        { "variants", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("foo"), },
                { "type", QLIT_QSTR("66"), },
                {}
            })),
#if defined(TEST_IF_UNION_BAR)
            QLIT_QDICT(((QLitDictEntry[]) {
                { "case", QLIT_QSTR("union_bar"), },
                { "type", QLIT_QSTR("46"), },
                {}
            })),
#endif /* defined(TEST_IF_UNION_BAR) */
            {}
        })), },
        {}
    })),
#endif /* defined(TEST_IF_UNION) && defined(TEST_IF_STRUCT) */
    /* "48" = TestIfAlternate */
#if defined(TEST_IF_ALT) && defined(TEST_IF_STRUCT)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("int"), },
                {}
            })),
#if defined(TEST_IF_ALT_BAR)
            QLIT_QDICT(((QLitDictEntry[]) {
                { "type", QLIT_QSTR("67"), },
                {}
            })),
#endif /* defined(TEST_IF_ALT_BAR) */
            {}
        })), },
        { "meta-type", QLIT_QSTR("alternate"), },
        { "name", QLIT_QSTR("48"), },
        {}
    })),
#endif /* defined(TEST_IF_ALT) && defined(TEST_IF_STRUCT) */
    /* "49" = TestIfStruct */
#if defined(TEST_IF_STRUCT)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("foo"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
#if defined(TEST_IF_STRUCT_BAR)
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("bar"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
#endif /* defined(TEST_IF_STRUCT_BAR) */
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("49"), },
        {}
    })),
#endif /* defined(TEST_IF_STRUCT) */
    /* "50" = TestIfEnum */
#if defined(TEST_IF_ENUM)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum"), },
        { "name", QLIT_QSTR("50"), },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("foo"),
#if defined(TEST_IF_ENUM_BAR)
            QLIT_QSTR("bar"),
#endif /* defined(TEST_IF_ENUM_BAR) */
            {}
        })), },
        {}
    })),
#endif /* defined(TEST_IF_ENUM) */
#if defined(TEST_IF_ENUM)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("50"), },
        { "meta-type", QLIT_QSTR("array"), },
        { "name", QLIT_QSTR("[50]"), },
        {}
    })),
#endif /* defined(TEST_IF_ENUM) */
    /* "51" = FeatureStruct0 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("foo"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("51"), },
        {}
    })),
    /* "52" = FeatureStruct1 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "features", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("feature1"),
            {}
        })), },
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("foo"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("52"), },
        {}
    })),
    /* "53" = FeatureStruct2 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "features", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("feature1"),
            {}
        })), },
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("foo"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("53"), },
        {}
    })),
    /* "54" = FeatureStruct3 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "features", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("feature1"),
            QLIT_QSTR("feature2"),
            {}
        })), },
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("foo"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("54"), },
        {}
    })),
    /* "55" = FeatureStruct4 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "features", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("namespace-test"),
            QLIT_QSTR("int"),
            QLIT_QSTR("name"),
            QLIT_QSTR("if"),
            {}
        })), },
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("namespace-test"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("55"), },
        {}
    })),
    /* "56" = CondFeatureStruct1 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "features", QLIT_QLIST(((QLitObject[]) {
#if defined(TEST_IF_FEATURE_1)
            QLIT_QSTR("feature1"),
#endif /* defined(TEST_IF_FEATURE_1) */
            {}
        })), },
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("foo"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("56"), },
        {}
    })),
    /* "57" = CondFeatureStruct2 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "features", QLIT_QLIST(((QLitObject[]) {
#if defined(TEST_IF_FEATURE_1)
            QLIT_QSTR("feature1"),
#endif /* defined(TEST_IF_FEATURE_1) */
#if defined(TEST_IF_FEATURE_2)
            QLIT_QSTR("feature2"),
#endif /* defined(TEST_IF_FEATURE_2) */
            {}
        })), },
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("foo"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("57"), },
        {}
    })),
    /* "58" = CondFeatureStruct3 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "features", QLIT_QLIST(((QLitObject[]) {
#if defined(TEST_IF_COND_1)
#if defined(TEST_IF_COND_2)
            QLIT_QSTR("feature1"),
#endif /* defined(TEST_IF_COND_2) */
#endif /* defined(TEST_IF_COND_1) */
            {}
        })), },
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("foo"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("58"), },
        {}
    })),
    /* "59" = UserDefTwoDictDict */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("userdef"), },
                { "type", QLIT_QSTR("21"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("59"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("int"), },
        { "meta-type", QLIT_QSTR("array"), },
        { "name", QLIT_QSTR("[int]"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("number"), },
        { "meta-type", QLIT_QSTR("array"), },
        { "name", QLIT_QSTR("[number]"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("number"), },
        { "meta-type", QLIT_QSTR("builtin"), },
        { "name", QLIT_QSTR("number"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("bool"), },
        { "meta-type", QLIT_QSTR("array"), },
        { "name", QLIT_QSTR("[bool]"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "json-type", QLIT_QSTR("boolean"), },
        { "meta-type", QLIT_QSTR("builtin"), },
        { "name", QLIT_QSTR("bool"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("str"), },
        { "meta-type", QLIT_QSTR("array"), },
        { "name", QLIT_QSTR("[str]"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("any"), },
        { "meta-type", QLIT_QSTR("array"), },
        { "name", QLIT_QSTR("[any]"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("60"), },
        { "meta-type", QLIT_QSTR("array"), },
        { "name", QLIT_QSTR("[60]"), },
        {}
    })),
    /* "60" = Status */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum"), },
        { "name", QLIT_QSTR("60"), },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("good"),
            QLIT_QSTR("bad"),
            QLIT_QSTR("ugly"),
            {}
        })), },
        {}
    })),
    /* "61" = UserDefA */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("boolean"), },
                { "type", QLIT_QSTR("bool"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("a_b"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("61"), },
        {}
    })),
    /* "62" = UserDefB */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("intb"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "default", QLIT_QNULL, },
                { "name", QLIT_QSTR("a-b"), },
                { "type", QLIT_QSTR("bool"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("62"), },
        {}
    })),
    /* "63" = __org.qemu_x-Struct2 */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("array"), },
                { "type", QLIT_QSTR("[14]"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("63"), },
        {}
    })),
    /* "64" = __org.qemu_x-Base */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("__org.qemu_x-member1"), },
                { "type", QLIT_QSTR("42"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("64"), },
        {}
    })),
    /* "65" = TestIfUnionKind */
#if defined(TEST_IF_UNION) && defined(TEST_IF_STRUCT)
    QLIT_QDICT(((QLitDictEntry[]) {
        { "meta-type", QLIT_QSTR("enum"), },
        { "name", QLIT_QSTR("65"), },
        { "values", QLIT_QLIST(((QLitObject[]) {
            QLIT_QSTR("foo"),
#if defined(TEST_IF_UNION_BAR)
            QLIT_QSTR("union_bar"),
#endif /* defined(TEST_IF_UNION_BAR) */
            {}
        })), },
        {}
    })),
#endif /* defined(TEST_IF_UNION) && defined(TEST_IF_STRUCT) */
    /* "66" = q_obj_TestStruct-wrapper */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("data"), },
                { "type", QLIT_QSTR("67"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("66"), },
        {}
    })),
    /* "67" = TestStruct */
    QLIT_QDICT(((QLitDictEntry[]) {
        { "members", QLIT_QLIST(((QLitObject[]) {
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("integer"), },
                { "type", QLIT_QSTR("int"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("boolean"), },
                { "type", QLIT_QSTR("bool"), },
                {}
            })),
            QLIT_QDICT(((QLitDictEntry[]) {
                { "name", QLIT_QSTR("string"), },
                { "type", QLIT_QSTR("str"), },
                {}
            })),
            {}
        })), },
        { "meta-type", QLIT_QSTR("object"), },
        { "name", QLIT_QSTR("67"), },
        {}
    })),
    QLIT_QDICT(((QLitDictEntry[]) {
        { "element-type", QLIT_QSTR("14"), },
        { "meta-type", QLIT_QSTR("array"), },
        { "name", QLIT_QSTR("[14]"), },
        {}
    })),
    {}
}));

/* Dummy declaration to prevent empty .o file */
char qapi_dummy_test_qapi_introspect_c;
