.global _start
.global __fdt_addr

_start:
    loop:
        addi x0, x0, 0
        j loop