
MicroPython
===========

Micropython is an implementation of the Python 3 language designed to run on embedded platforms. 
It provides a small subset of Python's standard library. 
More information is available on the `official site <https://micropython.org/>`__.

Integration of MicroPython in SO3
---------------------------------

A minimal port of micropython is available in SO3. This version only supports the basic features 
of the language and some basic modules (list below)


Using Micropython in the emulated environment
---------------------------------------------

.. note::

   Micropython currently works only for the virt64 platform
   
Micropython is built along with the rest of the user-space applications. The "usr/build.sh" script 
contains command lines to execute the Makefile found in the "usr/src/micropython/ports/soo" folder
and export the resulting "firmware.elf" executable in the "deploy" folder where all the other 
executables are sent. The executable is renamed "uPython.elf" at the same time for the sake of clarity
   
Once inside SO3, MicroPython can be launched like any other program::

   so3% uPython

Launching the program will open an interactive interpreter from which code may be tested. 
There is currently no way to execute a python script

Available `Micropython libraries <https://docs.micropython.org/en/latest/library/index.html#>`_ (modules):

* array
* collections
* struct
