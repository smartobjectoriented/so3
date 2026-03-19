#ifndef VFBDEV_H
#define VFBDEV_H

#include <soo/ring.h>
#include <soo/vdevfront.h>
#include <soo/gnttab.h>

#define VFBDEV_NAME "vfbdev"
#define VFBDEV_PREFIX "[" VFBDEV_NAME "] "

typedef struct {
	/* Nothing */
} vfbdev_request_t;

typedef struct {
	uint32_t hres;
	uint32_t vres;
	uint64_t size;
} vfbdev_response_t;

DEFINE_RING_TYPES(vfbdev, vfbdev_request_t, vfbdev_response_t);

typedef struct {
	/* Must be the first field */
	vdevfront_t vdevfront;

	vfbdev_front_ring_t ring;
	unsigned int irq;

	grant_ref_t ring_ref;
	uint32_t evtchn;
} vfbdev_t;

#endif /* VFBDEV_H */
