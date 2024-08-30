
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

   Micropython currently works only for the virt64 and rpi4_64 platforms
   
Micropython is built along with the rest of the user-space applications. To build the interpreter, got to the 'usr/CMakeLists.txt' file and enable the micropython compilation by modifying the line "option(MICROPYTHON "Support for micro-python apps" OFF)" from "OFF" to "ON". This will create the 'firmware.elf' application. At deploy this file is renamed "uPython.elf" for the sake of clarity
   
Once inside SO3, MicroPython can be launched like any other program::

   so3% uPython

Launching the program without any argument will open an interactive interpreter from which code may be tested. Exit it by pressing Ctrl+D

A script file may be given as an argument to be executed

Available `Micropython libraries <https://docs.micropython.org/en/latest/library/index.html#>`_ (modules):

* array
* collections
* struct
