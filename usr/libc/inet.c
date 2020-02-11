#include "inet.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

in_addr_t inet_addr(const char *cp)
{
    in_addr_t result = 0;

    int i;
    int j = 0;
    int k = 0;
    int l;
    char buf[4];
    char ip[4];
    int len = strlen(cp);
    char exp = 1;

    for (i = 0 ; i <= strlen(cp) ; i++) {
        if ((cp[i] == '.') || (i == len)) {
            buf[j] = '\0';
            j = 0;
            ip[k] = (char)0;
            exp = 1;
            for (l = strlen(buf) - 1 ; l >= 0 ; l--) {
                ip[k] += (char)((buf[l] - '0') * exp);
                exp *= (char)10;
            }
            k++;
        }
        else {
            buf[j] = cp[i];
            j++;
        }
    }

    result = (uint32_t)((uint32_t)(((ip[0] & 0xFF) << 24) & 0xFF000000) |
                        (uint32_t)(((ip[1] & 0xFF) << 16) & 0x00FF0000) |
                        (uint32_t)(((ip[2] & 0xFF) << 8) & 0x0000FF00) |
                        (uint32_t)((ip[3] & 0xFF) & 0x000000FF));

    return result;
}
