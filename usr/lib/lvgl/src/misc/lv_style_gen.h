static inline void lv_style_set_radius(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_RADIUS, v);
}

static inline void lv_style_set_clip_corner(lv_style_t * style, bool value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_CLIP_CORNER, v);
}

static inline void lv_style_set_transform_width(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_TRANSFORM_WIDTH, v);
}

static inline void lv_style_set_transform_height(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_TRANSFORM_HEIGHT, v);
}

static inline void lv_style_set_transform_zoom(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_TRANSFORM_ZOOM, v);
}

static inline void lv_style_set_transform_angle(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_TRANSFORM_ANGLE, v);
}

static inline void lv_style_set_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_OPA, v);
}

static inline void lv_style_set_color_filter_dsc(lv_style_t * style, const lv_color_filter_dsc_t * value)
{
    lv_style_value_t v = {
        .ptr = (void *)value
    };
    lv_style_set_prop(style, LV_STYLE_COLOR_FILTER_DSC, v);
}

static inline void lv_style_set_color_filter_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_COLOR_FILTER_OPA, v);
}

static inline void lv_style_set_anim_time(lv_style_t * style, uint32_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_ANIM_TIME, v);
}

static inline void lv_style_set_transition(lv_style_t * style, const lv_style_transition_dsc_t * value)
{
    lv_style_value_t v = {
        .ptr = value
    };
    lv_style_set_prop(style, LV_STYLE_TRANSITION, v);
}

static inline void lv_style_set_size(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_SIZE, v);
}

static inline void lv_style_set_blend_mode(lv_style_t * style, lv_blend_mode_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BLEND_MODE, v);
}

static inline void lv_style_set_pad_top(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_PAD_TOP, v);
}

static inline void lv_style_set_pad_bottom(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_PAD_BOTTOM, v);
}

static inline void lv_style_set_pad_left(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_PAD_LEFT, v);
}

static inline void lv_style_set_pad_right(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_PAD_RIGHT, v);
}

static inline void lv_style_set_pad_row(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_PAD_ROW, v);
}

static inline void lv_style_set_pad_column(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_PAD_COLUMN, v);
}

static inline void lv_style_set_bg_color(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_BG_COLOR, v);
}

static inline void lv_style_set_bg_color_filtered(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_BG_COLOR_FILTERED, v);
}

static inline void lv_style_set_bg_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BG_OPA, v);
}

static inline void lv_style_set_bg_grad_color(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_BG_GRAD_COLOR, v);
}

static inline void lv_style_set_bg_grad_color_filtered(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_BG_GRAD_COLOR_FILTERED, v);
}

static inline void lv_style_set_bg_grad_dir(lv_style_t * style, lv_grad_dir_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BG_GRAD_DIR, v);
}

static inline void lv_style_set_bg_main_stop(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BG_MAIN_STOP, v);
}

static inline void lv_style_set_bg_grad_stop(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BG_GRAD_STOP, v);
}

static inline void lv_style_set_bg_img_src(lv_style_t * style, const void * value)
{
    lv_style_value_t v = {
        .ptr = value
    };
    lv_style_set_prop(style, LV_STYLE_BG_IMG_SRC, v);
}

static inline void lv_style_set_bg_img_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BG_IMG_OPA, v);
}

static inline void lv_style_set_bg_img_recolor(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_BG_IMG_RECOLOR, v);
}

static inline void lv_style_set_bg_img_recolor_filtered(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_BG_IMG_RECOLOR_FILTERED, v);
}

static inline void lv_style_set_bg_img_recolor_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BG_IMG_RECOLOR_OPA, v);
}

static inline void lv_style_set_bg_img_tiled(lv_style_t * style, bool value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BG_IMG_TILED, v);
}

static inline void lv_style_set_border_color(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_BORDER_COLOR, v);
}

static inline void lv_style_set_border_color_filtered(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_BORDER_COLOR_FILTERED, v);
}

static inline void lv_style_set_border_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BORDER_OPA, v);
}

static inline void lv_style_set_border_width(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BORDER_WIDTH, v);
}

static inline void lv_style_set_border_side(lv_style_t * style, lv_border_side_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BORDER_SIDE, v);
}

static inline void lv_style_set_border_post(lv_style_t * style, bool value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_BORDER_POST, v);
}

static inline void lv_style_set_text_color(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_TEXT_COLOR, v);
}

static inline void lv_style_set_text_color_filtered(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_TEXT_COLOR_FILTERED, v);
}

static inline void lv_style_set_text_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_TEXT_OPA, v);
}

static inline void lv_style_set_text_font(lv_style_t * style, const lv_font_t * value)
{
    lv_style_value_t v = {
        .ptr = value
    };
    lv_style_set_prop(style, LV_STYLE_TEXT_FONT, v);
}

static inline void lv_style_set_text_letter_space(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_TEXT_LETTER_SPACE, v);
}

static inline void lv_style_set_text_line_space(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_TEXT_LINE_SPACE, v);
}

static inline void lv_style_set_text_decor(lv_style_t * style, lv_text_decor_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_TEXT_DECOR, v);
}

static inline void lv_style_set_text_align(lv_style_t * style, lv_text_align_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_TEXT_ALIGN, v);
}

static inline void lv_style_set_img_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_IMG_OPA, v);
}

static inline void lv_style_set_img_recolor(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_IMG_RECOLOR, v);
}

static inline void lv_style_set_img_recolor_filtered(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_IMG_RECOLOR_FILTERED, v);
}

static inline void lv_style_set_img_recolor_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_IMG_RECOLOR_OPA, v);
}

static inline void lv_style_set_outline_width(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_OUTLINE_WIDTH, v);
}

static inline void lv_style_set_outline_color(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_OUTLINE_COLOR, v);
}

static inline void lv_style_set_outline_color_filtered(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_OUTLINE_COLOR_FILTERED, v);
}

static inline void lv_style_set_outline_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_OUTLINE_OPA, v);
}

static inline void lv_style_set_outline_pad(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_OUTLINE_PAD, v);
}

static inline void lv_style_set_shadow_width(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_SHADOW_WIDTH, v);
}

static inline void lv_style_set_shadow_ofs_x(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_SHADOW_OFS_X, v);
}

static inline void lv_style_set_shadow_ofs_y(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_SHADOW_OFS_Y, v);
}

static inline void lv_style_set_shadow_spread(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_SHADOW_SPREAD, v);
}

static inline void lv_style_set_shadow_color(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_SHADOW_COLOR, v);
}

static inline void lv_style_set_shadow_color_filtered(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_SHADOW_COLOR_FILTERED, v);
}

static inline void lv_style_set_shadow_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_SHADOW_OPA, v);
}

static inline void lv_style_set_line_width(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_LINE_WIDTH, v);
}

static inline void lv_style_set_line_dash_width(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_LINE_DASH_WIDTH, v);
}

static inline void lv_style_set_line_dash_gap(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_LINE_DASH_GAP, v);
}

static inline void lv_style_set_line_rounded(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_LINE_ROUNDED, v);
}

static inline void lv_style_set_line_color(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_LINE_COLOR, v);
}

static inline void lv_style_set_line_color_filtered(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_LINE_COLOR_FILTERED, v);
}

static inline void lv_style_set_line_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_LINE_OPA, v);
}

static inline void lv_style_set_arc_width(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_ARC_WIDTH, v);
}

static inline void lv_style_set_arc_rounded(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_ARC_ROUNDED, v);
}

static inline void lv_style_set_arc_color(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_ARC_COLOR, v);
}

static inline void lv_style_set_arc_color_filtered(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_ARC_COLOR_FILTERED, v);
}

static inline void lv_style_set_arc_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_ARC_OPA, v);
}

static inline void lv_style_set_arc_img_src(lv_style_t * style, const void * value)
{
    lv_style_value_t v = {
        .ptr = value
    };
    lv_style_set_prop(style, LV_STYLE_ARC_IMG_SRC, v);
}

static inline void lv_style_set_content_text(lv_style_t * style, const char * value)
{
    lv_style_value_t v = {
        .ptr = value
    };
    lv_style_set_prop(style, LV_STYLE_CONTENT_TEXT, v);
}

static inline void lv_style_set_content_align(lv_style_t * style, lv_align_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_CONTENT_ALIGN, v);
}

static inline void lv_style_set_content_ofs_x(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_CONTENT_OFS_X, v);
}

static inline void lv_style_set_content_ofs_y(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_CONTENT_OFS_Y, v);
}

static inline void lv_style_set_content_opa(lv_style_t * style, lv_opa_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_CONTENT_OPA, v);
}

static inline void lv_style_set_content_font(lv_style_t * style, const lv_font_t * value)
{
    lv_style_value_t v = {
        .ptr = value
    };
    lv_style_set_prop(style, LV_STYLE_CONTENT_FONT, v);
}

static inline void lv_style_set_content_color(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_CONTENT_COLOR, v);
}

static inline void lv_style_set_content_color_filtered(lv_style_t * style, lv_color_t value)
{
    lv_style_value_t v = {
        .color = value
    };
    lv_style_set_prop(style, LV_STYLE_CONTENT_COLOR_FILTERED, v);
}

static inline void lv_style_set_content_letter_space(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_CONTENT_LETTER_SPACE, v);
}

static inline void lv_style_set_content_line_space(lv_style_t * style, lv_coord_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_CONTENT_LINE_SPACE, v);
}

static inline void lv_style_set_content_decor(lv_style_t * style, lv_text_decor_t value)
{
    lv_style_value_t v = {
        .num = (int32_t)value
    };
    lv_style_set_prop(style, LV_STYLE_CONTENT_DECOR, v);
}

