//
// Created by julien on 3/12/20.
//

#ifndef SO3_CC_H
#define SO3_CC_H





#ifndef LWIP_PLATFORM_DIAG
#define LWIP_PLATFORM_DIAG(x) printk x
#include <printk.h>
#endif

#ifndef LWIP_PLATFORM_ASSERT
#define LWIP_PLATFORM_ASSERT(x) do {printk("Assertion \"%s\" failed at line %d in %s\n", \
                                         x, __LINE__, __FILE__); fflush(NULL); abort();} while(0)

#include <printk.h>
#endif


#define LWIP_NO_STDINT_H 1
#define LWIP_NO_INTTYPES_H 1


#define X8_F  "02x"
#define U16_F "d"
#define S16_F "d"
#define X16_F "x"
#define U32_F "d"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "lu"

#include <types.h>
typedef u8      u8_t;
typedef signed char s8_t;
typedef u16     u16_t;
typedef s16     s16_t;
typedef u32     u32_t;
typedef s32     s32_t;

typedef unsigned long int  mem_ptr_t;

#endif //SO3_CC_H
