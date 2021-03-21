#include "../../../lvgl.h"
#if LV_USE_CHECKBOX && LV_BUILD_EXAMPLES

static void event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_VALUE_CHANGED) {
        const char * txt = lv_checkbox_get_text(obj);
        const char * state = lv_obj_get_state(obj) & LV_STATE_CHECKED ? "Checked" : "Unchecked";
        LV_LOG_USER("%s: %s\n", txt, state);
    }
}

void lv_example_checkbox_1(void)
{
    static lv_flex_t flex_center;
    lv_flex_init(&flex_center);
    lv_flex_set_flow(&flex_center, LV_FLEX_FLOW_COLUMN);
    lv_flex_set_place(&flex_center, LV_FLEX_PLACE_CENTER, LV_FLEX_PLACE_START, LV_FLEX_PLACE_CENTER);

    lv_obj_set_layout(lv_scr_act(), &flex_center);

    lv_obj_t * cb;
    cb = lv_checkbox_create(lv_scr_act(), NULL);
    lv_checkbox_set_text(cb, "Apple");
    lv_obj_add_event_cb(cb, event_handler, NULL);

    cb = lv_checkbox_create(lv_scr_act(), NULL);
    lv_checkbox_set_text(cb, "Banana");
    lv_obj_add_state(cb, LV_STATE_CHECKED);
    lv_obj_add_event_cb(cb, event_handler, NULL);

    cb = lv_checkbox_create(lv_scr_act(), NULL);
    lv_checkbox_set_text(cb, "Lemon");
    lv_obj_add_state(cb, LV_STATE_DISABLED);
    lv_obj_add_event_cb(cb, event_handler, NULL);

    cb = lv_checkbox_create(lv_scr_act(), NULL);
    lv_obj_add_state(cb, LV_STATE_CHECKED | LV_STATE_DISABLED);
    lv_checkbox_set_text(cb, "Melon");
    lv_obj_add_event_cb(cb, event_handler, NULL);
}

#endif
