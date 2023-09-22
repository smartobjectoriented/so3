# The SOO port

This port is intended to be a minimal MicroPython port that runs under 
Linux in the [SOO project](https://github.com/smartobjectoriented/soo)  

## Building and running Linux version

When building from the SOO repository, you can simply run

    $ make

Once the program is built, you can deploy it using

    $ make deploy

After deploying into and running Linux in QEMU, you can launch the app
using the following command

    $ saved_=`stty -g`; \
	  stty raw opost -echo; \
	  ./upython; \
	  echo "Exit status: $$?"; \
	  stty $$saved_

Exiting the interactive interpreter is done using Ctrl+D

## Building without the built-in MicroPython compiler

This minimal port can be built with the built-in MicroPython compiler
disabled.  This will reduce the firmware by about 20k on a Thumb2 machine,
and by about 40k on 32-bit x86.  Without the compiler the REPL will be
disabled, but pre-compiled scripts can still be executed.

To test out this feature, change the `MICROPY_ENABLE_COMPILER` config
option to "0" in the mpconfigport.h file in this directory.  Then
recompile and run the firmware and it will execute the frozentest.py
file.
