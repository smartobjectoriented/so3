    .section .upgrade_image_section,#alloc
    .globl  upgrade_image
upgrade_image:
    .incbin  "./update.bin"
upgrade_image_end:

    .globl upgrade_image_length
upgrade_image_length:
    .word  upgrade_image_end - upgrade_image
