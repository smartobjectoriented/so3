/**
 * @file lv_spinner.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_spinner.h"
#if LV_USE_SPINNER

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void arc_anim_start_angle(void * obj, int32_t v);
static void arc_anim_end_angle(void * obj, int32_t v);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Create a spinner object
 * @param par pointer to an object, it will be the parent of the new spinner
 * @param copy pointer to a spinner object, if not NULL then the new object will be copied from
 * it
 * @return pointer to the created spinner
 */
lv_obj_t * lv_spinner_create(lv_obj_t * par, uint32_t time, uint32_t arc_length)
{
    /*Create the ancestor of spinner*/
    lv_obj_t * spinner = lv_arc_create(par, NULL);
    LV_ASSERT_MALLOC(spinner);
    if(spinner == NULL) return NULL;

    lv_obj_set_size(spinner, LV_DPI_DEF, LV_DPI_DEF);

    lv_obj_remove_style(spinner, LV_PART_KNOB, LV_STATE_ANY, NULL);

    lv_anim_path_t path;
    lv_anim_path_init(&path);
    lv_anim_path_set_cb(&path, lv_anim_path_ease_in_out);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, spinner);
    lv_anim_set_exec_cb(&a, arc_anim_end_angle);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_time(&a, time);
    lv_anim_set_values(&a, arc_length, 360 + arc_length);
    lv_anim_start(&a);

    lv_anim_set_path(&a, &path);
    lv_anim_set_values(&a, 0, 360);
    lv_anim_set_exec_cb(&a, arc_anim_start_angle);
    lv_anim_start(&a);

    lv_arc_set_bg_angles(spinner, 0, 360);
    lv_arc_set_rotation(spinner, 270);

    return spinner;
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

static void arc_anim_start_angle(void * obj, int32_t v)
{
    lv_arc_set_start_angle(obj, (uint16_t) v);
}


static void arc_anim_end_angle(void * obj, int32_t v)
{
    lv_arc_set_end_angle(obj, (uint16_t) v);
}

#endif /*LV_USE_SPINNER*/
