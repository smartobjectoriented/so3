
include_directories(.)

#add_compile_options("-DLV_LVGL_H_INCLUDE_SIMPLE")

add_compile_options("-DLV_USE_DEMO_WIDGETS") 
add_executable(demofb.elf demofb.c lv_demo_widgets.c img_lvgl_logo.c img_demo_widgets_avatar.c img_clothes.c)

target_link_libraries(demofb.elf c lvgl)