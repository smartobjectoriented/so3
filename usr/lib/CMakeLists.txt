# Hacky fix to make sure we don't compile external libs with -Werror flag
set(ORIGINAL_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE "-Werror" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

add_subdirectory(libc)
set(LV_BUILD_CONF_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE PATH "Directory containing lv_conf.h")
add_subdirectory(lvgl)

set(CMAKE_C_FLAGS "${ORIGINAL_C_FLAGS}")

# Non external libs
add_subdirectory(slv)

# logs functionality - it only works with SOO environment
add_subdirectory(logs)
