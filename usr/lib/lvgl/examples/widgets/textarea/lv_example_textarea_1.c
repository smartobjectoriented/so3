#include "../../../lvgl.h"
#if LV_USE_TEXTAREA && LV_BUILD_EXAMPLES


static void btnm_event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t * ta = lv_event_get_user_data();
        const char * txt = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_selected_btn(obj));

        if(strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) lv_textarea_del_char(ta);
        else if(strcmp(txt, LV_SYMBOL_NEW_LINE) == 0) lv_textarea_add_char(ta, '\n');
        else lv_textarea_add_text(ta, txt);

    }
}

void lv_example_textarea_1(void)
{
    lv_obj_t * ta = lv_textarea_create(lv_scr_act(), NULL);
    lv_textarea_set_one_line(ta, true);
    lv_obj_align(ta, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
    lv_obj_add_state(ta, LV_STATE_FOCUSED); /*To be sure the cursor is visible*/

    static const char * btnm_map[] = {"1", "2", "3", "\n",
                               "4", "5", "6", "\n",
                               "7", "8", "9", "\n",
                               LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_NEW_LINE, ""};

    lv_obj_t * btnm = lv_btnmatrix_create(lv_scr_act(), NULL);
    lv_obj_set_size(btnm, 200, 150);
    lv_obj_align(btnm, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btnm, btnm_event_handler, ta);
    lv_obj_clear_flag(btnm, LV_OBJ_FLAG_CLICK_FOCUSABLE); /*To keep the text area focused on button clicks*/
    lv_btnmatrix_set_map(btnm, btnm_map);
}

#endif
