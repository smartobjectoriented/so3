/**
 * @file lv_win.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_win.h"
#if LV_USE_WIN


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_win_constructor(lv_obj_t * obj, const lv_obj_t * copy);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_win_class = {.constructor_cb = lv_win_constructor, .base_class = &lv_obj_class, .instance_size = sizeof(lv_win_t)};
static lv_coord_t create_header_height;
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_win_create(lv_obj_t * parent, lv_coord_t header_height)
{
    create_header_height = header_height;
    return lv_obj_create_from_class(&lv_win_class, parent, NULL);
}

lv_obj_t * lv_win_add_title(lv_obj_t * win, const char * txt)
{
    lv_obj_t * header = lv_win_get_header(win);
    lv_obj_t * title = lv_label_create(header, NULL);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_label_set_text(title, txt);
    lv_obj_set_flex_grow(title, 1);
    return title;
}

lv_obj_t * lv_win_add_btn(lv_obj_t * win, const void * icon, lv_coord_t btn_w, lv_event_cb_t event_cb)
{
    lv_obj_t * header = lv_win_get_header(win);
    lv_obj_t * btn = lv_btn_create(header, NULL);
    lv_obj_set_size(btn, btn_w, LV_SIZE_PCT(100));
    lv_obj_add_event_cb(btn, event_cb, NULL);

    lv_obj_t * img = lv_img_create(btn, NULL);
    lv_img_set_src(img, icon);
    lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);

    return btn;
}

lv_obj_t * lv_win_get_header(lv_obj_t * win)
{
    return lv_obj_get_child(win, 0);
}

lv_obj_t * lv_win_get_content(lv_obj_t * win)
{
    return lv_obj_get_child(win, 1);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_win_constructor(lv_obj_t * obj, const lv_obj_t * copy)
{
    LV_UNUSED(copy);
    lv_obj_t * parent = lv_obj_get_parent(obj);
    lv_obj_set_size(obj, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_set_layout(obj, &lv_flex_column_nowrap);

    lv_obj_t * header = lv_obj_create(obj, NULL);
    lv_obj_set_size(header, LV_SIZE_PCT(100), create_header_height);
    lv_obj_set_layout(header, &lv_flex_row_wrap);

    lv_obj_t * cont = lv_obj_create(obj, NULL);
    lv_obj_set_flex_grow(cont, 1);
    lv_obj_set_width(cont, LV_SIZE_PCT(100));
}

#endif

