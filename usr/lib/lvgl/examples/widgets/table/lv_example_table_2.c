#include "../../../lvgl.h"
#if LV_USE_TABLE && LV_BUILD_EXAMPLES

#define ITEM_CNT 200

static void event_cb(lv_obj_t * obj, lv_event_t e)
{
    if(e == LV_EVENT_DRAW_PART_END) {
        lv_obj_draw_dsc_t * dsc = lv_event_get_param();
        /*If the cells are drawn...*/
        if(dsc->part == LV_PART_ITEMS) {
            bool chk = lv_table_has_cell_ctrl(obj, dsc->id, 0, LV_TABLE_CELL_CTRL_CUSTOM_1);

            lv_draw_rect_dsc_t rect_dsc;
            lv_draw_rect_dsc_init(&rect_dsc);
            rect_dsc.bg_color = chk ? lv_theme_get_color_primary(obj) : lv_color_grey_lighten_2();
            rect_dsc.radius = LV_RADIUS_CIRCLE;

            lv_area_t sw_area;
            sw_area.x1 = dsc->draw_area->x2 - 50;
            sw_area.x2 = sw_area.x1 + 40;
            sw_area.y1 =  dsc->draw_area->y1 + lv_area_get_height(dsc->draw_area) / 2 - 10;
            sw_area.y2 = sw_area.y1 + 20;
            lv_draw_rect(&sw_area, dsc->clip_area, &rect_dsc);

            rect_dsc.bg_color = lv_color_white();
            if(chk) {
                sw_area.x2 -= 2;
                sw_area.x1 = sw_area.x2 - 16;
            } else {
                sw_area.x1 += 2;
                sw_area.x2 = sw_area.x1 + 16;
            }
            sw_area.y1 += 2;
            sw_area.y2 -= 2;
            lv_draw_rect(&sw_area, dsc->clip_area, &rect_dsc);
        }
    }
    else if(e == LV_EVENT_VALUE_CHANGED) {
        uint16_t col;
        uint16_t row;
        lv_table_get_selected_cell(obj, &row, &col);
        bool chk = lv_table_has_cell_ctrl(obj, row, 0, LV_TABLE_CELL_CTRL_CUSTOM_1);
        if(chk) lv_table_clear_cell_ctrl(obj, row, 0, LV_TABLE_CELL_CTRL_CUSTOM_1);
        else lv_table_add_cell_ctrl(obj, row, 0, LV_TABLE_CELL_CTRL_CUSTOM_1);
    }
}


/**
 * A very light-weighted list created from table
 */
void lv_example_table_2(void)
{
    /*Measure memory usage*/
    lv_mem_monitor_t mon1;
    lv_mem_monitor(&mon1);

    uint32_t t = lv_tick_get();

    lv_obj_t * table = lv_table_create(lv_scr_act(), NULL);

    /*Set a smaller height to the table. It'll make it scrollable*/
    lv_obj_set_size(table, 150, 200);

    lv_table_set_col_width(table, 0, 150);
    lv_table_set_row_cnt(table, ITEM_CNT); /*Not required but avoids a lot of memory reallocation lv_table_set_set_value*/
    lv_table_set_col_cnt(table, 1);

    /*Don't make the cell pressed, we will draw something different in the event*/
    lv_obj_remove_style(table, LV_PART_ITEMS, LV_STATE_PRESSED, NULL);

    uint32_t i;
    for(i = 0; i < ITEM_CNT; i++) {
        lv_table_set_cell_value_fmt(table, i, 0, "Item %d", i + 1);
    }

    lv_obj_align(table, NULL, LV_ALIGN_CENTER, 0, -20);

    /*Add an event callback to to apply some custom drawing*/
    lv_obj_add_event_cb(table, event_cb, NULL);

    lv_mem_monitor_t mon2;
    lv_mem_monitor(&mon2);

    uint32_t mem_used = mon1.free_size - mon2.free_size;

    uint32_t elaps = lv_tick_elaps(t);

    lv_obj_t * label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text_fmt(label, "%d bytes are used by the table\n"
                                  "and %d items were added in %d ms",
                                  mem_used, ITEM_CNT, elaps);

    lv_obj_align(label, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -10);

}

#endif
