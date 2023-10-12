
add_custom_command(OUTPUT firmware.elf
        DEPENDS c
        WORKING_DIRECTORY  ${PROJECT_SOURCE_DIR}/src/micropython
        COMMAND /bin/sh ${PROJECT_SOURCE_DIR}/src/micropython/build.sh ${PROJECT_BINARY_DIR}/src/micropython
)

add_custom_target(uPython.elf ALL
  DEPENDS firmware.elf
  COMMAND mv ${PROJECT_BINARY_DIR}/src/micropython/firmware.elf ${PROJECT_BINARY_DIR}/src/micropython/uPython.elf
)

