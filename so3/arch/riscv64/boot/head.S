.global _start
.global __fdt_addr

.section ".head.text","ax"

_start:
    loop:
        addi x0, x0, 1
        j loop
