/**
 * @file lv_obj.h
 *
 */

#ifndef LV_OBJ_H
#define LV_OBJ_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lv_conf_internal.h"

#include <stddef.h>
#include <stdbool.h>
#include "../misc/lv_style.h"
#include "../misc/lv_types.h"
#include "../misc/lv_area.h"
#include "../misc/lv_color.h"
#include "../misc/lv_assert.h"
#include "../hal/lv_hal.h"

/*********************
 *      DEFINES
 *********************/
#define _LV_EVENT_FLAG_BUBBLED 0x80

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_obj_t;

/*---------------------
 *       EVENTS
 *---------------------*/

/**
 * Type of event being sent to the object.
 */
typedef enum {
    /** Input device events*/
    LV_EVENT_PRESSED,             /**< The object has been pressed*/
    LV_EVENT_PRESSING,            /**< The object is being pressed (called continuously while pressing)*/
    LV_EVENT_PRESS_LOST,          /**< User is still being pressed but slid cursor/finger off of the object */
    LV_EVENT_SHORT_CLICKED,       /**< User pressed object for a short period of time, then released it. Not called if scrolled.*/
    LV_EVENT_LONG_PRESSED,        /**< Object has been pressed for at least `LV_INDEV_LONG_PRESS_TIME`.  Not called if scrolled.*/
    LV_EVENT_LONG_PRESSED_REPEAT, /**< Called after `LV_INDEV_LONG_PRESS_TIME` in every `LV_INDEV_LONG_PRESS_REP_TIME` ms.  Not called if scrolled.*/
    LV_EVENT_CLICKED,             /**< Called on release if not scrolled (regardless to long press)*/
    LV_EVENT_RELEASED,            /**< Called in every cases when the object has been released*/
    LV_EVENT_SCROLL_BEGIN,        /**< Scrolling begins*/
    LV_EVENT_SCROLL_END,          /**< Scrolling ends*/
    LV_EVENT_SCROLL,              /**< Scrolling*/
    LV_EVENT_GESTURE,             /**< Gesture detected*/
    LV_EVENT_KEY,                 /**< A key is sent to the object*/
    LV_EVENT_FOCUSED,             /**< Focused */
    LV_EVENT_DEFOCUSED,           /**< Defocused*/
    LV_EVENT_LEAVE,               /**< Defocused but still selected*/
    LV_EVENT_HIT_TEST,            /**< Perform advanced hit-testing*/

    /** Drawing events*/
    LV_EVENT_COVER_CHECK,        /**< Check if the object fully covers an area*/
    LV_EVENT_REFR_EXT_DRAW_SIZE, /**< Get the required extra draw area around the object (e.g. for shadow)*/
    LV_EVENT_DRAW_MAIN_BEGIN,    /**< Starting the main drawing phase*/
    LV_EVENT_DRAW_MAIN,          /**< Perform the main drawing*/
    LV_EVENT_DRAW_MAIN_END,      /**< Finishing the main drawing phase*/
    LV_EVENT_DRAW_POST_BEGIN,    /**< Starting the post draw phase (when all children are drawn)*/
    LV_EVENT_DRAW_POST,          /**< Perform the post draw phase (when all children are drawn)*/
    LV_EVENT_DRAW_POST_END,      /**< Finishing the post draw phase (when all children are drawn)*/
    LV_EVENT_DRAW_PART_BEGIN,    /**< Starting to draw a part*/
    LV_EVENT_DRAW_PART_END,      /**< Finishing to draw a part*/

    /** General events*/
    LV_EVENT_VALUE_CHANGED,       /**< The object's value has changed (i.e. slider moved)*/
    LV_EVENT_INSERT,              /**< A text is inserted to the object*/
    LV_EVENT_REFRESH,             /**< Notify the object to refresh something on it (for the user)*/
    LV_EVENT_DELETE,              /**< Object is being deleted*/
    LV_EVENT_READY,               /**< A process has finished*/
    LV_EVENT_CANCEL,              /**< A process has been cancelled */
    LV_EVENT_CHILD_CHANGED,       /**< Child was removed/added*/
    LV_EVENT_COORD_CHANGED,       /**< Object coordinates/size have changed*/
    LV_EVENT_STYLE_CHANGED,       /**< Object's style has changed*/
    LV_EVENT_BASE_DIR_CHANGED,    /**< The base dir has changed*/
    LV_EVENT_GET_SELF_SIZE,       /**< Get the internal size of a widget*/

    _LV_EVENT_LAST /** Number of default events*/
}lv_event_t;

/**
 * @brief Event callback.
 * Events are used to notify the user of some action being taken on the object.
 * For details, see ::lv_event_t.
 */
typedef void (*lv_event_cb_t)(struct _lv_obj_t * obj, lv_event_t event);

typedef struct {
    lv_event_cb_t cb;
    void * user_data;
}lv_event_dsc_t;

/*---------------------
 *       EVENTS
 *---------------------*/

/**
 * Possible states of a widget.
 * OR-ed values are possible
 */
enum {
    LV_STATE_DEFAULT     =  0x0000,
    LV_STATE_CHECKED     =  0x0001,
    LV_STATE_FOCUSED     =  0x0002,
    LV_STATE_FOCUS_KEY   =  0x0004,
    LV_STATE_EDITED      =  0x0008,
    LV_STATE_HOVERED     =  0x0010,
    LV_STATE_PRESSED     =  0x0020,
    LV_STATE_SCROLLED    =  0x0040,
    LV_STATE_DISABLED    =  0x0080,

    LV_STATE_USER_1      =  0x1000,
    LV_STATE_USER_2      =  0x2000,
    LV_STATE_USER_3      =  0x4000,
    LV_STATE_USER_4      =  0x8000,

    LV_STATE_ANY = 0xFFFF,    /**< Special value can be used in some functions to target all states*/
};

typedef uint16_t lv_state_t;

/**
 * The possible parts of widgets.
 * The parts can be considered as the internal building block of the widgets.
 * E.g. slider = background + indicator + knob
 * Note every part is used by every widget
 */
enum {
    LV_PART_MAIN,        /**< A background like rectangle*/
    LV_PART_SCROLLBAR,   /**< The scrollbar(s)*/
    LV_PART_INDICATOR,   /**< Indicator, e.g. for slider, bar, switch, or the tick box of the checkbox*/
    LV_PART_KNOB,        /**< Like handle to grab to adjust the value*/
    LV_PART_SELECTED,    /**< Indicate the currently selected option or section*/
    LV_PART_ITEMS,       /**< Used if the widget has multiple similar elements (e.g. tabel cells)*/
    LV_PART_TICKS,       /**< Ticks on scale e.g. for a chart or meter*/
    LV_PART_CURSOR,      /**< Mark a specific place e.g. for text area's cursor or on a chart*/

    LV_PART_CUSTOM_1 = 0x40,    /**< Extension point for custom widgets*/
    LV_PART_CUSTOM_2,           /**< Extension point for custom widgets*/
    LV_PART_CUSTOM_3,           /**< Extension point for custom widgets*/
    LV_PART_CUSTOM_4,           /**< Extension point for custom widgets*/
    LV_PART_CUSTOM_5,           /**< Extension point for custom widgets*/
    LV_PART_CUSTOM_6,           /**< Extension point for custom widgets*/
    LV_PART_CUSTOM_7,           /**< Extension point for custom widgets*/
    LV_PART_CUSTOM_8,           /**< Extension point for custom widgets*/

    LV_PART_ANY = 0xFF,  /**< Special value can be used in some functions to target all parts*/
};

typedef uint16_t lv_part_t;

/**
 * On/Off features controlling the object's behavior.
 * OR-ed values are possible
 */
enum {
    LV_OBJ_FLAG_HIDDEN          = (1 << 0),  /**< Make the object hidden. (Like it wasn't there at all)*/
    LV_OBJ_FLAG_CLICKABLE       = (1 << 1),  /**< Make the object clickable by the input devices*/
    LV_OBJ_FLAG_CLICK_FOCUSABLE = (1 << 2),  /**< Add focused state to the object when clicked*/
    LV_OBJ_FLAG_CHECKABLE       = (1 << 3),  /**< Toggle checked state when the object is clicked*/
    LV_OBJ_FLAG_SCROLLABLE      = (1 << 4),  /**< Make the object scrollable*/
    LV_OBJ_FLAG_SCROLL_ELASTIC  = (1 << 5),  /**< Allow scrolling inside but with slower speed*/
    LV_OBJ_FLAG_SCROLL_MOMENTUM = (1 << 6),  /**< Make the object scroll further when "thrown"*/
    LV_OBJ_FLAG_SCROLL_ONE      = (1 << 7),   /**< Allow scrolling only one snapable children*/
    LV_OBJ_FLAG_SCROLL_CHAIN    = (1 << 8),  /**< Allow propagating the scroll to a parent*/
    LV_OBJ_FLAG_SCROLL_ON_FOCUS = (1 << 9),  /**< Automatically scroll object to make it visible when focused*/
    LV_OBJ_FLAG_SNAPABLE        = (1 << 10), /**< If scroll snap is enabled on the parent it can snap to this object*/
    LV_OBJ_FLAG_PRESS_LOCK      = (1 << 11), /**< Keep the object pressed even if the press slid from the object*/
    LV_OBJ_FLAG_EVENT_BUBBLE    = (1 << 12), /**< Propagate the events to the parent too*/
    LV_OBJ_FLAG_GESTURE_BUBBLE  = (1 << 13), /**< Propagate the gestures to the parent*/
    LV_OBJ_FLAG_FOCUS_BUBBLE    = (1 << 14), /**< Propagate the focus to the parent*/
    LV_OBJ_FLAG_ADV_HITTEST     = (1 << 15), /**< Allow performing more accurate hit (click) test. E.g. consider rounded corners.*/
    LV_OBJ_FLAG_IGNORE_LAYOUT   = (1 << 16), /**< Make the object position-able by the layouts*/
    LV_OBJ_FLAG_FLOATING        = (1 << 17), /**< Do not scroll the object when the parent scrolls and ignore layout*/

    LV_OBJ_FLAG_LAYOUT_1        = (1 << 23), /** Custom flag, free to use by layouts*/
    LV_OBJ_FLAG_LAYOUT_2        = (1 << 24), /** Custom flag, free to use by layouts*/

    LV_OBJ_FLAG_WIDGET_1        = (1 << 25), /** Custom flag, free to use by widget*/
    LV_OBJ_FLAG_WIDGET_2        = (1 << 26), /** Custom flag, free to use by widget*/

    LV_OBJ_FLAG_USER_1          = (1 << 27), /** Custom flag, free to use by user*/
    LV_OBJ_FLAG_USER_2          = (1 << 28), /** Custom flag, free to use by user*/
    LV_OBJ_FLAG_USER_3          = (1 << 29), /** Custom flag, free to use by user*/
    LV_OBJ_FLAG_USER_4          = (1 << 30), /** Custom flag, free to use by user*/
};
typedef uint32_t lv_obj_flag_t;

#include "lv_obj_tree.h"
#include "lv_obj_pos.h"
#include "lv_obj_scroll.h"
#include "lv_obj_style.h"
#include "lv_obj_draw.h"
#include "lv_obj_class.h"
#include "lv_group.h"

/**
 * Make the base object's class publicly available.
 */
extern const lv_obj_class_t lv_obj_class;

/**
 * Special, rarely used attributes.
 * They are allocated automatically if any elements is set.
 */
typedef struct {
    struct _lv_obj_t ** children;       /**< Store the pointer of the children in an array.*/
    uint32_t child_cnt;                 /**< Number of children*/
    lv_group_t * group_p;

    const lv_layout_dsc_t * layout_dsc; /**< Pointer to the layout descriptor*/

    lv_event_dsc_t * event_dsc;             /**< Dynamically allocated event callback and user data array*/
    lv_point_t scroll;                      /**< The current X/Y scroll offset*/

    uint8_t ext_click_pad;      /**< Extra click padding in all direction*/
    lv_coord_t ext_draw_size;           /**< EXTend the size in every direction for drawing.*/

    lv_scrollbar_mode_t scrollbar_mode :2; /**< How to display scrollbars*/
    lv_scroll_snap_t scroll_snap_x : 2;      /**< Where to align the snapable children horizontally*/
    lv_scroll_snap_t scroll_snap_y : 2;      /**< Where to align the snapable children horizontally*/
    lv_dir_t scroll_dir :4;                /**< The allowed scroll direction(s)*/
    lv_bidi_dir_t base_dir  : 2; /**< Base direction of texts related to this object*/
    uint8_t event_dsc_cnt;           /**< Number of event callabcks stored in `event_cb` array*/
}lv_obj_spec_attr_t;

typedef struct _lv_obj_t{
    const lv_obj_class_t * class_p;
    struct _lv_obj_t * parent;
    lv_obj_spec_attr_t * spec_attr;
    lv_obj_style_t * styles;
    lv_area_t coords;
    lv_coord_t x_set;
    lv_coord_t y_set;
    lv_coord_t w_set;
    lv_coord_t h_set;
    lv_obj_flag_t flags;
    lv_state_t state;
    uint8_t layout_inv :1;
    uint8_t skip_trans :1;
    uint8_t style_cnt  :6;
}lv_obj_t;


typedef struct {
    const lv_point_t * point;
    bool result;
} lv_hit_test_info_t;

typedef struct {
    lv_draw_res_t res;
    const lv_area_t * clip_area;
} lv_cover_check_info_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize LVGL library.
 * Should be called before any other LVGL related function.
 */
void lv_init(void);

#if LV_ENABLE_GC || !LV_MEM_CUSTOM

/**
 * Deinit the 'lv' library
 * Currently only implemented when not using custom allocators, or GC is enabled.
 */
void lv_deinit(void);

#endif

/**
 * Create a base object (a rectangle)
 * @param parent    pointer to a parent object. If NULL then a screen will be created.
 * @param copy      DEPRECATED, will be removed in v9.
 *                  Pointer to an other base object to copy.
 * @return pointer to the new object
 */
lv_obj_t * lv_obj_create(lv_obj_t * parent, const lv_obj_t * copy);


/*---------------------
 * Event/Signal sending
 *---------------------*/

/**
 * Send an event to the object
 * @param obj pointer to an object
 * @param event the type of the event from `lv_event_t`
 * @param param arbitrary data depending on the object type and the event. (Usually `NULL`)
 * @return LV_RES_OK: `obj` was not deleted in the event; LV_RES_INV: `obj` was deleted in the event
 */
lv_res_t lv_event_send(lv_obj_t * obj, lv_event_t event, void * param);

lv_res_t lv_obj_event_base(const lv_obj_class_t * class_p, struct _lv_obj_t * obj, lv_event_t e);

/**
 * Get the `param` parameter of the current event
 * @return      the `param` parameter
 */
void * lv_event_get_param(void);

/**
 * Get the user data of the event callback. (Set when the callback is registered)
 * @return      the user data parameter
 */
void * lv_event_get_user_data(void);

/**
 * Register a new, custom event ID.
 * It can be used the same way as e.g. `LV_EVENT_CLICKED` to send custom events
 * @return      the new event id
 * @example
 * uint32_t LV_EVENT_MINE = 0;
 * ...
 * e = lv_event_register_id();
 * ...
 * lv_event_send(obj, LV_EVENT_MINE, &some_data);
 */
uint32_t lv_event_register_id(void);

/**
 * Nested events can be called and one of them might belong to an object that is being deleted.
 * Mark this object's `event_temp_data` deleted to know that it's `lv_event_send` should return `LV_RES_INV`
 * @param obj pointer to an obejct to mark as deleted
 */
void _lv_event_mark_deleted(lv_obj_t * obj);

/*=====================
 * Setter functions
 *====================*/

/**
 * Set one or more flags
 * @param obj   pointer to an object
 * @param f     R-ed values from `lv_obj_flag_t` to set.
 */
void lv_obj_add_flag(lv_obj_t * obj, lv_obj_flag_t f);

/**
 * Clear one or more flags
 * @param obj   pointer to an object
 * @param f     OR-ed values from `lv_obj_flag_t` to set.
 */
void lv_obj_clear_flag(lv_obj_t * obj, lv_obj_flag_t f);


/**
 * Add one or more states to the object. The other state bits will remain unchanged.
 * If specified in the styles, transition animation will be started from the previous state to the current.
 * @param obj       pointer to an object
 * @param state     the states to add. E.g `LV_STATE_PRESSED | LV_STATE_FOCUSED`
 */
void lv_obj_add_state(lv_obj_t * obj, lv_state_t state);

/**
 * Remove one or more states to the object. The other state bits will remain unchanged.
 * If specified in the styles, transition animation will be started from the previous state to the current.
 * @param obj       pointer to an object
 * @param state     the states to add. E.g `LV_STATE_PRESSED | LV_STATE_FOCUSED`
 */
void lv_obj_clear_state(lv_obj_t * obj, lv_state_t state);

/**
 * Add an event handler function for an object.
 * Used by the user to react on event which happens with the object.
 * An object can have multiple event handler. They will be called in the same order as they were added.
 * @param obj       pointer to an object
 * @param event_cb  the new event function
 * @param user_data custom data data will be available in `event_cb`
 */
void lv_obj_add_event_cb(lv_obj_t * obj, lv_event_cb_t event_cb, void * user_data);

/**
 * Remove an event handler function for an object.
 * Used by the user to react on event which happens with the object.
 * An object can have multiple event handler. They will be called in the same order as they were added.
 * @param obj       pointer to an object
 * @param event_cb  the event function to remove
 * @param user_data if NULL, remove the first event handler with the same cb, otherwise remove the first event handler with the same cb and user_data
 * @return true if any event handlers were removed
 */
bool lv_obj_remove_event_cb(lv_obj_t * obj, lv_event_cb_t event_cb, void * user_data);

/**
 * Set the base direction of the object
 * @param obj   pointer to an object
 * @param dir   the new base direction. `LV_BIDI_DIR_LTR/RTL/AUTO/INHERIT`
 */
void lv_obj_set_base_dir(lv_obj_t * obj, lv_bidi_dir_t dir);


/*=======================
 * Getter functions
 *======================*/

/**
 * Check if a given flag or all the given flags are set on an object.
 * @param obj   pointer to an object
 * @param f     the flag(s) to check (OR-ed values can be used)
 * @return      true: all flags are set; false: not all flags are set
 */
bool lv_obj_has_flag(const lv_obj_t * obj, lv_obj_flag_t f);

/**
 * Check if a given flag or any of the flags are set on an object.
 * @param obj   pointer to an object
 * @param f     the flag(s) to check (OR-ed values can be used)
 * @return      true: at lest one flag flag is set; false: none of the flags are set
 */
bool lv_obj_has_flag_any(const lv_obj_t * obj, lv_obj_flag_t f);


/**
 * Get the base direction of the object
 * @param obj   pointer to an object
 * @return      the base direction. `LV_BIDI_DIR_LTR/RTL/AUTO/INHERIT`
 */
lv_bidi_dir_t lv_obj_get_base_dir(const lv_obj_t * obj);

/**
 * Get the state of an object
 * @param obj   pointer to an object
 * @return      the state (OR-ed values from `lv_state_t`)
 */
lv_state_t lv_obj_get_state(const lv_obj_t * obj);

/**
 * Check if the object is in a given state or not.
 * @param obj       pointer to an object
 * @param state     a state or combination of states to check
 * @return          true: `obj` is in `state`; false: `obj` is not in `state`
 */
bool lv_obj_has_state(const lv_obj_t * obj, lv_state_t state);

/**
 * Get the event function of an object
 * @param obj   pointer to an object
 * @param id    the index of the event callback. 0: the firstly added
 * @return      the event descriptor
 */
lv_event_dsc_t * lv_obj_get_event_dsc(const lv_obj_t * obj, uint32_t id);

/**
 * Get the group of the object
 * @param       obj pointer to an object
 * @return      the pointer to group of the object
 */
void * lv_obj_get_group(const lv_obj_t * obj);

/*=======================
 * Other functions
 *======================*/

/**
 * Allocate special data for an object if not allocated yet.
 * @param obj   pointer to an object
 */
void lv_obj_allocate_spec_attr(lv_obj_t * obj);

/**
 * Get the focused object by taking `LV_OBJ_FLAG_FOCUS_BUBBLE` into account.
 * @param obj   the start object
 * @return      the object to to really focus
 */
lv_obj_t * lv_obj_get_focused_obj(const lv_obj_t * obj);

/**
 * Get object's and its ancestors type. Put their name in `type_buf` starting with the current type.
 * E.g. buf.type[0]="lv_btn", buf.type[1]="lv_cont", buf.type[2]="lv_obj"
 * @param obj   pointer to an object which type should be get
 * @param buf   pointer to an `lv_obj_type_t` buffer to store the types
 */
bool lv_obj_check_type(const lv_obj_t * obj, const lv_obj_class_t * class_p);

/**
 * Check if any object has a given type
 * @param obj       pointer to an object
 * @param obj_type  type of the object. (e.g. "lv_btn")
 * @return          true: valid
 */
bool lv_obj_has_class(const lv_obj_t * obj, const lv_obj_class_t * class_p);

/**
 * Check if any object is still "alive", and part of the hierarchy
 * @param obj       pointer to an object
 * @param obj_type  type of the object. (e.g. "lv_btn")
 * @return          true: valid
 */
bool lv_obj_is_valid(const lv_obj_t * obj);

/**********************
 *      MACROS
 **********************/

#if LV_USE_ASSERT_OBJ
#  define LV_ASSERT_OBJ(obj_p, obj_class)                                    \
            LV_ASSERT_MSG(obj_p != NULL, "The object is NULL");               \
            LV_ASSERT_MSG(lv_obj_has_class(obj_p, obj_class) == true, "Incompatible object type."); \
            LV_ASSERT_MSG(lv_obj_is_valid(obj_p)  == true, "The object is invalid, deleted or corrupted?");

# else
# define LV_ASSERT_OBJ(obj_p, obj_class) do{}while(0)
#endif

#if LV_USE_LOG && LV_LOG_TRACE_OBJ_CREATE
#  define LV_TRACE_OBJ_CREATE(...) LV_LOG_TRACE( __VA_ARGS__)
#else
#  define LV_TRACE_OBJ_CREATE(...)
#endif


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_OBJ_H*/
