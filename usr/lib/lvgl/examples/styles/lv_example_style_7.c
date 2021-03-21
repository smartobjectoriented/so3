#include "../../lvgl.h"
#if LV_BUILD_EXAMPLES && LV_USE_LABEL

/**
 * Using the text style properties
 */
void lv_example_style_7(void)
{
    static lv_style_t style;
    lv_style_init(&style);

    lv_style_set_radius(&style, 5);
    lv_style_set_bg_opa(&style, LV_OPA_COVER);
    lv_style_set_bg_color(&style, lv_color_grey_lighten_3());
    lv_style_set_border_width(&style, 2);
    lv_style_set_border_color(&style, lv_color_blue());
    lv_style_set_pad_all(&style, 10);

    lv_style_set_text_color(&style, lv_color_blue());
    lv_style_set_text_letter_space(&style, 5);
    lv_style_set_text_line_space(&style, 20);
    lv_style_set_text_decor(&style, LV_TEXT_DECOR_UNDERLINE);

    /*Create an object with the new style*/
    lv_obj_t * obj = lv_label_create(lv_scr_act(), NULL);
    lv_obj_add_style(obj, LV_PART_MAIN, LV_STATE_DEFAULT, &style);
    lv_label_set_text(obj, "Text of\n"
                            "a label");

    lv_obj_align(obj, NULL, LV_ALIGN_CENTER, 0, 0);
}

#endif
