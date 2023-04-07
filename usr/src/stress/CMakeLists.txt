
include_directories(.)

#add_compile_options("-DLV_LVGL_H_INCLUDE_SIMPLE")

add_compile_options("-DLV_USE_DEMO_STRESS") 

add_executable(stress.elf lvgl_perf.c lv_demo_stress.c)


target_link_libraries(stress.elf c lvgl)
