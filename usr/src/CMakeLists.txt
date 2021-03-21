 
add_executable(sh.elf sh.c)
add_executable(demofb.elf demofb.c)

target_link_libraries(sh.elf c)
target_link_libraries(demofb.elf c lvgl)

