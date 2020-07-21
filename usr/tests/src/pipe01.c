// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 */

/*
 * Basic test for pipe().
 */

#include <stdio.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>
//#include "tst_test.h"

static int fds[2];

//static void verify_pipe(void)
int main()
{
        int pipe_result = 0;
    
	int rd_size, wr_size;
	char wrbuf[] = "abcdefghijklmnopqrstuvwxyz";
	char rdbuf[128];

	memset(rdbuf, 0, sizeof(rdbuf));

	//TEST(pipe(fds));
        errno = 0;
        pipe_result = pipe(fds);

	//if (TST_RET == -1) {
        if (pipe_result < 0) {
                //tst_res(TFAIL | TTERRNO, "pipe()");
                printf("[FAIL] pipe() test failed with error %d\n", errno);
		return -1;
	}

	//wr_size = SAFE_WRITE(1, fds[1], wrbuf, sizeof(wrbuf));
        wr_size = write(fds[1], wrbuf, sizeof(wrbuf));
	//rd_size = SAFE_READ(0, fds[0], rdbuf, sizeof(rdbuf));
        rd_size = read(fds[0], rdbuf, sizeof(rdbuf));

	if (rd_size != wr_size) {
                //tst_res(TFAIL, "read() returned %d, expected %d",
		//        rd_size, wr_size);
                printf("[FAIL] read() returned %d, expected %d\n",
                       rd_size, wr_size);
		return -1;
	}

	if ((strncmp(rdbuf, wrbuf, wr_size)) != 0) {
                //tst_res(TFAIL, "Wrong data were read back");
                printf("[FAIL] Wrong data were read back\n");
		return -1;
	}

	//SAFE_CLOSE(fds[0]);
        close(fds[0]);
	//SAFE_CLOSE(fds[1]);
        close(fds[1]);

	//tst_res(TPASS, "pipe() functionality is correct");
        printf("[PASS] pipe() functionnality is correct\n");
        return 0;
}

//static struct tst_test test = {
//	.test_all = verify_pipe,
//};
