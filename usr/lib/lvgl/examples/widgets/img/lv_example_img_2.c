#include "../../../lvgl.h"
#if LV_USE_IMG && LV_USE_SLIDER && LV_BUILD_EXAMPLES

static lv_obj_t * create_slider(lv_color_t color);
static void slider_event_cb(lv_obj_t * slider, lv_event_t event);

static lv_obj_t * red_slider, * green_slider, * blue_slider, * intense_slider;
static lv_obj_t * img1;


/**
 * Demonstrate runtime image re-coloring
 */
void lv_example_img_2(void)
{
    /*Create 4 sliders to adjust RGB color and re-color intensity*/
    red_slider = create_slider(lv_color_red());
    green_slider = create_slider(lv_color_green());
    blue_slider = create_slider(lv_color_blue());
    intense_slider = create_slider(lv_color_grey());

    lv_slider_set_value(red_slider, LV_OPA_20, LV_ANIM_OFF);
    lv_slider_set_value(green_slider, LV_OPA_90, LV_ANIM_OFF);
    lv_slider_set_value(blue_slider, LV_OPA_60, LV_ANIM_OFF);
    lv_slider_set_value(intense_slider, LV_OPA_50, LV_ANIM_OFF);

    lv_obj_align(red_slider, NULL, LV_ALIGN_IN_LEFT_MID, 25, 0);
    lv_obj_align(green_slider, red_slider, LV_ALIGN_OUT_RIGHT_MID, 25, 0);
    lv_obj_align(blue_slider, green_slider, LV_ALIGN_OUT_RIGHT_MID, 25, 0);
    lv_obj_align(intense_slider, blue_slider, LV_ALIGN_OUT_RIGHT_MID, 25, 0);

    /*Now create the actual image*/
    LV_IMG_DECLARE(img_cogwheel_argb)
    img1 = lv_img_create(lv_scr_act(), NULL);
    lv_img_set_src(img1, &img_cogwheel_argb);
    lv_obj_align(img1, NULL, LV_ALIGN_IN_RIGHT_MID, -20, 0);

    lv_event_send(intense_slider, LV_EVENT_VALUE_CHANGED, NULL);
}

static void slider_event_cb(lv_obj_t * slider, lv_event_t event)
{
    LV_UNUSED(slider);

    if(event == LV_EVENT_VALUE_CHANGED) {
        /*Recolor the image based on the sliders' values*/
        lv_color_t color  = lv_color_make(lv_slider_get_value(red_slider), lv_slider_get_value(green_slider), lv_slider_get_value(blue_slider));
        lv_opa_t intense = lv_slider_get_value(intense_slider);
        lv_obj_set_style_img_recolor_opa(img1, LV_PART_MAIN, LV_STATE_DEFAULT, intense);
        lv_obj_set_style_img_recolor(img1, LV_PART_MAIN, LV_STATE_DEFAULT, color);
    }
}

static lv_obj_t * create_slider(lv_color_t color)
{
    lv_obj_t * slider = lv_slider_create(lv_scr_act(), NULL);
    lv_slider_set_range(slider, 0, 255);
    lv_obj_set_size(slider, 10, 200);
    lv_obj_set_style_bg_color(slider, LV_PART_KNOB, LV_STATE_DEFAULT, color);
    lv_obj_set_style_bg_color(slider, LV_PART_INDICATOR, LV_STATE_DEFAULT, lv_color_darken(color, LV_OPA_40));
    lv_obj_add_event_cb(slider, slider_event_cb, NULL);
    return slider;

}

#endif
