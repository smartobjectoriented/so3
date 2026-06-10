.. _debugging:

Debugging SO3
#############

Because SO3 is developed primarily on QEMU, the emulator's built-in **GDB stub**
is the day-to-day debugging tool. This chapter shows the QEMU/GDB workflow; for
debugging on real hardware see :ref:`Debugging with JTAG on Raspberry Pi 4
<so3_jtag_rpi4>`.

Debug symbols
=============

The linked kernel ``so3/so3`` is an ELF file with full symbols (the binary
flashed into the FIT image is the stripped ``so3.bin``). Point GDB at the ELF to
get source-level debugging while the target runs the matching ``.bin``.

Attaching GDB to QEMU
=====================

The launch scripts already expose a GDB stub. ``./st`` (and ``./stv``) start
QEMU with ``-gdb tcp::1234``; add ``-S`` to make QEMU **wait** for the debugger
before executing the first instruction. A minimal headless invocation looks
like:

.. code-block:: bash

   sudo qemu-system-aarch64 -smp 4 -M virt -cpu cortex-a72 -m 1024 \
       -kernel u-boot/u-boot -nographic -no-reboot \
       -drive if=none,file=filesystem/sdcard.img.virt64,id=hd0,format=raw \
       -device virtio-blk-device,drive=hd0 \
       -S -gdb tcp::1234

Then, from the ``so3/`` directory:

.. code-block:: bash

   gdb-multiarch -q \
       -ex 'file so3' \
       -ex 'target remote :1234' \
       -ex 'break kernel_start' \
       -ex 'continue'

.. tip::

   Use **gdb-multiarch** (or an ``aarch64`` GDB). The repository ships a small
   ``gdbinit`` with sensible defaults (for example a bounded backtrace limit).

Useful breakpoints
==================

A few entry points are handy when tracing a boot or a fault:

.. flat-table::
   :header-rows: 1
   :widths: 38 62

   * - Symbol
     - What it marks
   * - ``kernel_start``
     - start of kernel bring-up
   * - ``memory_init`` / ``devices_init``
     - memory and device-tree initialisation
   * - ``trap_handle`` / ``trap_handle_error``
     - the synchronous-exception path (faults, syscalls)
   * - ``el01_sync_handler`` / ``syscall_handle``
     - the system-call path
   * - ``create_root_process`` / ``ret_from_fork``
     - the transition into the first user process

Inspecting an exception
=======================

When a fault is taken, the kernel prints the offending ``ELR``, ``FAR`` and
``ESR`` and stops. To find the faulting instruction in the ELF, add the kernel
virtual base to the load offset and disassemble:

.. code-block:: bash

   aarch64-none-elf-objdump -d so3/so3 | less    # search for the address

The top bits of ``ESR`` give the exception class (``EC``); a value of ``0x15`` is
an ``SVC`` (system call), ``0x20`` / ``0x21`` an instruction abort, ``0x24`` /
``0x25`` a data abort, and ``0x00`` an "unknown reason" trap (often an
instruction that is illegal at the current exception level — a frequent symptom
when EL2-only code runs at EL1).

.. note::

   QEMU's GDB stub cannot always read the AArch64 system registers by name. When
   ``$elr_el1`` and friends are unavailable, read the **saved exception frame**
   off the stack instead: within ``struct cpu_regs`` the offsets are
   ``OFFSET_PC = 256``, ``OFFSET_PSTATE = 264`` and ``OFFSET_SP_USR = 272``
   (with ``S_FRAME_SIZE = 288`` in the standalone build).

Debugging AVZ
=============

For the hypervisor, launch with ``./stv`` (so EL2 is available) and add ``-S
-gdb tcp::1234`` the same way. Load symbols from the hypervisor ELF
(``avz/so3``) for the EL2 code and from the agency ELF (``so3/so3``) for the EL1
guest. The AVZ console trace (the *Loading Guest Domain* section and the vGIC
messages) is usually the quickest way to locate a problem before reaching for
the debugger.

Semihosting
===========

The ARM **semihosting** interface (``arch/arm64/semihosting.c``) is available
under the emulator and through a JTAG probe, allowing early ``printf``-style
output and host file access before the console driver is up.

On real hardware
================

For JTAG-based debugging on the Raspberry Pi 4 (OpenOCD + GDB over a hardware
probe), follow the dedicated chapter: :ref:`so3_jtag_rpi4`.
