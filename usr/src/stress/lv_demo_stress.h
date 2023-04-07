/**
 * @file lv_demo_stress.h
 *
 */

#ifndef LV_DEMO_STRESS_H
#define LV_DEMO_STRESS_H

#include <lvgl.h>

#define LVGL_BUF_SIZE (256 * 256)

/* Framebuffer ioctl commands. */

#define IOCTL_FB_HRES 1
#define IOCTL_FB_VRES 2
#define IOCTL_FB_SIZE 3

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_demos.h"

/*********************
 *      DEFINES
 *********************/

#define LV_DEMO_STRESS_TIME_STEP    50

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_demo_stress(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_STRESS_H*/
