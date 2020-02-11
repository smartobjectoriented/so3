/*
 * types.h
 *
 *  Created on: Sep 5, 2013
 *      Author: romain
 */

#ifndef TYPES_H_
#define TYPES_H_

#include <bits/alltypes.h>

typedef unsigned int mode_t;
typedef int pid_t;

typedef uint32_t in_addr_t;

struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr_in {
    u_short         sin_family;
    u_short         sin_port;
    struct in_addr  sin_addr;
    char            sin_zero[8];
};

typedef uint32_t socklen_t;

#endif /* TYPES_H_ */
