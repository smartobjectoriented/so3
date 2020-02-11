#ifndef LIBC_INET_H_
#define LIBC_INET_H_

#include "types.h"

/* Supported socket address families. */
#define AF_INET         2       /* Internet IP Protocol */

/**
 * enum sock_type - Socket types
 */
enum sock_type {
    SOCK_STREAM     = 1,
    SOCK_DGRAM      = 2,
    SOCK_RAW        = 3,
    SOCK_RDM        = 4,
    SOCK_SEQPACKET  = 5,
    SOCK_DCCP       = 6,
    SOCK_PACKET     = 10,
};

/**
 * enum socket_protocol - IP protocols
 */
enum socket_protocol {
    IPPROTO_IP      = 0,
    IPPROTO_ICMP    = 1,
    IPPROTO_IGMP    = 2,
    IPPROTO_IPIP    = 4,
    IPPROTO_TCP     = 6,
    IPPROTO_EGP     = 8,
    IPPROTO_PUP     = 12,
    IPPROTO_UDP     = 17,
    IPPROTO_IDP     = 22,
    IPPROTO_DCCP    = 33,
    IPPROTO_RSVP    = 46,
    IPPROTO_GRE     = 47,
    IPPROTO_IPV6    = 41,
    IPPROTO_ESP     = 50,
    IPPROTO_AH      = 51,
    IPPROTO_BEETPH  = 94,
    IPPROTO_PIM     = 103,
    IPPROTO_COMP    = 108,
    IPPROTO_SCTP    = 132,
    IPPROTO_UDPLITE = 136,
    IPPROTO_RAW     = 255,
    IPPROTO_MAX
};

in_addr_t inet_addr(const char *cp);

#endif /* LIBC_INET_H_ */
