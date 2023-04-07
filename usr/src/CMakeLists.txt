
add_executable(sh.elf sh.c)
add_executable(ls.elf ls.c)
add_executable(more.elf more.c)
add_executable(time.elf time.c)
add_executable(ping.elf ping.c)

add_subdirectory(widgets)
add_subdirectory(stress)

target_link_libraries(sh.elf c)
target_link_libraries(ls.elf c)
target_link_libraries(more.elf c)
target_link_libraries(time.elf c)
target_link_libraries(ping.elf c)
