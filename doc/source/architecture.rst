.. _architecture:

SO3 Architecture
================

.. figure:: img/SO3_Architecture.png
   :scale: 50 %
    
   Overview of the SO3 environment with user and kernel space

*to be completed*

SO3 Kernel Space
----------------

*to be completed*

SO3 User Space
--------------

All user space files are located in ``usr/`` directory. Applications and the *libc* is in separated
directories.


The MUSL libc user space library
--------------------------------

SO3 integrates the `MUSL library <MUSL_libc_>`__ as *libc* for its user space application.

Not all functions are available in SO3, but the functions are enabled as soon as there is a necessity to have it.
Furthermore, more complex functions such as those used to manage ``pthreads`` for example are not activated since
only a minimal set of functionalities and features are present.
 

*to be completed*


.. _MUSL_libc: https://musl.libc.org
