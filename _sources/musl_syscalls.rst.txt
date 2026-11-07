.. _musl_syscalls:

What MUSL expects of the kernel
###############################

SO3's user space runs on an **unmodified** MUSL, built from source with
``musl-cross-make`` by the ``musl-toolchain`` recipe (``meta-toolchain``). The
kernel meets MUSL where Linux does: the system calls MUSL makes, with their
numbers, arguments and behaviour, and the process start-up it expects. This
alignment was done in the MICOFE project, funded by the
`Hasler Foundation <https://haslerstiftung.ch/en/welcome-to-the-hasler-foundation>`__;
SO3 carried its own MUSL fork before.

The ABI is Linux's: the number in ``x8`` (``r7`` on 32-bit ARM), the
arguments in ``x0``–``x5``, the result, or a negative ``errno``, in ``x0``.
MUSL turns the negative value into ``errno`` and ``-1`` itself, so the kernel
no longer shares an ``errno`` variable with user space.

Start-up
========

MUSL's entry point finds on the stack, as Linux lays it out: ``argc``, the
``argv[]`` array, the ``envp[]`` array, each ``NULL``-terminated, and the
auxiliary vector (``aux[]``, pairs of an identifier and a value, such as the
page size and the program headers). It then sets its thread pointer: the TLS
of the thread, in ``tpidr_el0`` on AArch64, which the kernel saves and
restores with the thread.

The system calls
================

``syscall.tbl`` lists every system call SO3 provides, by its Linux name;
``scripts/syscall_gen.sh`` turns it, with the architecture's numbers taken
from MUSL, into the dispatch table (see :ref:`kernel`). A function is
declared and defined with ``SYSCALL_DECLARE`` and ``SYSCALL_DEFINEn``, after
Linux's macros, which cast the arguments.

A system call SO3 lacks returns ``-ENOSYS``, and the kernel prints its number
(``unhandled syscall``): a new application shows at once what it misses. A few
which MUSL calls but SO3 has no use for are mapped to an empty function, which
returns ``-ENOSYS`` silently (``statx``, ``munmap``, ``mprotect``).

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Group
     - System calls, and what SO3 leaves out
   * - Files
     - ``read[v]``, ``write[v]``, ``open[at]``, ``close``, ``[_l]lseek``,
       ``ioctl``, ``dup[2,3]``, ``getdents64``, ``fcntl[64]``,
       ``stat64``, ``fstatat64``, ``newfstatat``, ``chdir``, ``getcwd``,
       ``mkdirat``, ``unlinkat``, ``renameat[2]``, ``utimensat``.
       ``renameat2`` and ``utimensat`` ignore their flags.
   * - Memory
     - ``mmap``, ``mmap2``, ``brk``. ``mmap`` serves anonymous mappings
       (``MAP_ANONYMOUS``), which MUSL's allocator asks for.
   * - Processes and threads
     - ``fork``, ``clone``, ``execve``, ``wait4``, ``exit``, ``exit_group``,
       ``getpid``, ``set_tid_address``, ``futex``, ``sched_yield``.
       ``clone`` makes threads with the flags MUSL's ``pthread_create``
       passes; ``futex`` has ``FUTEX_WAIT`` and ``FUTEX_WAKE`` only, and
       ignores the timeout.
   * - Signals
     - ``rt_sigaction``, ``kill``, ``sigreturn``, ``rt_sigreturn``,
       ``rt_sigprocmask``.
   * - Pipes
     - ``pipe``, ``pipe2``.
   * - Time
     - ``nanosleep``, ``gettimeofday[_time32]``, ``clock_gettime[32,64]``.
   * - Network
     - ``socket``, ``connect``, ``bind``, ``listen``, ``accept``,
       ``recv[from]``, ``send[to]``, ``setsockopt``.

Threads
=======

SO3 had system calls of its own to create, join and end a thread. MUSL's
``pthread`` uses Linux's instead, and so does SO3 now:

* ``clone``, which returns 0 in the new thread, on the stack and with the TLS
  its caller gives: MUSL's wrapper calls the thread's function;
* ``futex``, on which mutexes, condition variables and joining wait;
* ``set_tid_address`` and ``CLONE_CHILD_CLEARTID``: the kernel clears the
  thread's identifier and wakes its waiters when it ends, which is how
  ``pthread_join`` returns;
* ``exit`` ends one thread, ``exit_group`` the whole process.

C++
===

The same toolchain builds C++: constructors of global objects run before
``main``, ``libgcc`` brings the compiler's helpers, and the standard
library's threads sit on MUSL's ``pthread``. The programs of
``so3/usr/src/tests/`` exercise classes, exceptions and dynamic allocation
(see :ref:`user_space`).
