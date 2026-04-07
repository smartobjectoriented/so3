#ifndef FBDEV_GNT_H
#define FBDEV_GNT_H

#include <soo/uapi/soo.h>

void fbdev_set_pgtable(struct domain *d, int slotID);
void fbdev_set_info(fbdev_info_t *fbdev);
void fbdev_change_focus(int new_slotID);
addr_t fbdev_get_addr(void);

#endif /* FBDEV_GNT_H */
