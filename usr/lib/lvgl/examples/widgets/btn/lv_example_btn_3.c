#include "../../../lvgl.h"
#include <stdio.h>
#if LV_USE_BTN

/**
 * Create a style transition on a button to act like a gum when clicked
 */
void lv_example_btn_3(void)
{
    /*Properties to transition*/
    static lv_style_prop_t props[] = {
            LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_TRANSFORM_HEIGHT, LV_STYLE_CONTENT_LETTER_SPACE, 0
    };

    /*Define animation paths*/
    static lv_anim_path_t path_ease_in_out;
    lv_anim_path_init(&path_ease_in_out);
    lv_anim_path_set_cb(&path_ease_in_out, lv_anim_path_ease_in_out);

    static lv_anim_path_t path_overshoot;
    lv_anim_path_init(&path_overshoot);
    lv_anim_path_set_cb(&path_overshoot, lv_anim_path_overshoot);


    /*Transition descriptor when going back to the default state.
     *Add some delay to be sure the press transition is visible even if the press was very short*/
    static lv_style_transition_dsc_t transition_dsc_def;
    lv_style_transition_dsc_init(&transition_dsc_def, props, &path_overshoot, 250, 100);

    /*Transition descriptor when going to pressed state.
     *No delay, go to presses state immediately*/
    static lv_style_transition_dsc_t transition_dsc_pr;
    lv_style_transition_dsc_init(&transition_dsc_pr, props, &path_ease_in_out, 250, 0);

    /*Add only the new transition to he default state*/
    static lv_style_t style_def;
    lv_style_init(&style_def);
    lv_style_set_transition(&style_def, &transition_dsc_def);

    /*Add the transition and some transformation to the presses state.*/
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_transform_width(&style_pr, 10);
    lv_style_set_transform_height(&style_pr, -10);
    lv_style_set_content_letter_space(&style_pr, 10);
    lv_style_set_transition(&style_pr, &transition_dsc_pr);


    lv_obj_t * btn1 = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_align(btn1, NULL, LV_ALIGN_CENTER, 0, -80);
    lv_obj_add_style(btn1, LV_PART_MAIN, LV_STATE_PRESSED, &style_pr);
    lv_obj_add_style(btn1, LV_PART_MAIN, LV_STATE_DEFAULT, &style_def);

    /*Instead of creating a label add a values string*/
    lv_obj_set_style_content_text(btn1, LV_PART_MAIN, LV_STATE_DEFAULT, "Gum");
}
#endif
