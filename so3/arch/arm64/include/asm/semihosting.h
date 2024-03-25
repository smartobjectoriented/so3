/*
 * Copyright (c) 2012 Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Linaro Limited nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 */

#ifndef SEMIHOSTING_H
#define SEMIHOSTING_H

#include <types.h>

#define SYS_OPEN 1
#define OPEN_RDONLY 1
#define OPEN_WRONLY 4
#define SYS_CLOSE 2
#define SYS_WRITEC 3
#define SYS_WRITE0 4
#define SYS_WRITE 5
#define SYS_READ 6
#define SYS_ISTTY 9
#define SYS_SEEK 0x0A
#define SYS_FLEN 0x0C
#define SYS_REMOVE 0x0E
#define SYS_GET_CMDLINE 0x15
#define SYS_REPORTEXC 0x18
#define REPORTEXC_REASON_APP_EXIT 0x20026
#define SYS_EXIT_EXTENDED 0x20
#define SEMIHOSTING_SVC 0x123456 /* SVC comment field for semihosting */

#define FEATURE_DETECT_FILE ":semihosting-features"
#define SHFB_MAGIC_0 0x53
#define SHFB_MAGIC_1 0x48
#define SHFB_MAGIC_2 0x46
#define SHFB_MAGIC_3 0x42
#define SH_EXT_EXIT_EXTENDED (1 << 0)
#define SH_EXT_STDOUT_STDERR (1 << 1)

#ifndef __ASSEMBLER__

int __semi_call(int id, ...);
int semi_open(char const *filename, int mode);
int semi_close(int fd);
int semi_write0(char const *string);
void semi_writec(char c);
int semi_read(int fd, char *buffer, int length);
int semi_write(int fd, const char *buffer, int length);
int semi_flen(int fd);
int semi_istty(int fd);
int semi_seek(int fd, intptr_t pos);
int semi_remove(const char *filename);
int semi_get_cmdline(char *buffer, int size, int *length);
int semi_reportexc(int reason, int subcode);
void semi_fatal(char const *message);
void semi_exit(int subcode);
void semi_exit_extended(int subcode);
/*
 * semi_load_file:
 * On entry *dest should be the buffer to write the data to, and
 * *size should be the size of that buffer.
 * On success, return 0, *size is updated to the size of the data read,
 * and *dest is advanced to point to the end of the loaded data
 */
int semi_load_file(void **dest, unsigned *size, char const *filename);

#endif /* ! __ASSEMBLER__ */

#endif /* ! SEMIHOSTING_H */
