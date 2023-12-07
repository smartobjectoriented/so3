
LVGL - Light and Versatile Embedded Graphics Library
====================================================

LVGL is a free and open-source library providing an efficient GUI for embedded systems.
More information is available on the `official site <https://lvgl.io/>`__.

Integration of LVGL in SO3
--------------------------

The major work of porting LVGL in SO3 has been done by Nikolaos Garanis in the context of 
his `Bachelor project <https://nyg.gitlab.io/so3-support-graphique>`_.

Some details about the porting can be found in our `discussion forum <https://discourse.heig-vd.ch/t/graphics-support-for-so3/41/18>`__.

There is `a small video <LVGL_qemu_>`__ to show LVGL running in the QEMU/vExpress framebuffer emulated environment.

In addition, from LVGL v8, the ``lv_demo_widgets`` is now fully supported. And yes, SO3 integrates LVGL v8.

This part will be completed very soon...


.. _LVGL_qemu: https://youtu.be/skn_mp3ZBhI

Using LVGL in the emulated environment
--------------------------------------

.. note::

   First, make sure you compiled the kernel with a configuration
   which has the framebuffer enabled (for example, *vexpress_fb_defconfig*)
   
In order to have a graphical framebuffer in the emulated QEMU/vExpress 
environment, it is necessary to start the emulator with the ``stg`` script:

.. code-block:: bash

   ./stg
   
QEMU will start a new GUI window used as framebuffer.

LVGL Performance Test
---------------------

SO3 is used to check the performance of LVGL. LVGL's repo provides a "check_perf" workflow that 

#. Retrieves LVGL 8.3 and stores it in a "lvgl_base" folder for the dockerfile to use
#. Retrieves "Dockerfile.lvgl" from the SO3 repository and builds the docker image
#. Runs the docker image
#. Creates an artifact in the form of a log file that shows the output of the perf test 

The image is ran using two volumes: One that redirects the container's "/host" folder to the workflow's working directory and one that allows the container to access the workflow's devices (in the /dev folder) as his own (NOTE: is it really necessary ?)

The workflow is setup to run when
* Commits are pushed to LVGL's repo 
* A pull request is created 
* Launched from another workflow

Dockerfile
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

LVGL's check_perf workflow uses the Dockerfile.lvgl found at the root of this repository to create the image that runs SO3 on qemu and executes the tests. The dockerfile does the following:

#. Creates an Alpine image and installs all the necessary tools (like gcc and qemu)
#. Recovers SO3's main branch in the root ("/") folder 
#. Empties the "*/so3/usr/lib/lvgl*" folder and replaces its content with the LVGL repo to be tested (The LVGL code should be in a "lvgl_base" folder)
#. (Finds and removes all files related to "thumb")
#. Patches SO3 so it executes the *stress* application instead of the shell
#. Builds U-boot and the SO3 kernel
#. Imports the stress project into SO3's userspace and builds the userspace
#. Sets up the image so it exposes the port 1234 when ran and executes "./st"

Performance report [WIP]
^^^^^^^^^^^^^^^^^^^^^^^^

**Objective**: 

Run procedures to calculate the execution time of library functions and raise alarm when it is:

* Beyond set threshold (fail workflow ?)
* (Higher than in previous version)

**Code instrumentation options**: 

* **Unity**: LVGL uses Unity as a unit test framework but it doesn't provide any macro for execution time calculation
* **GProf**: gprof is a gcc utility used by adding the "-pg" option at compile (and linking) time. It instrumentalises the code automatically to calculate the execution time of each function and to create a file containing informations during the execution. Once the file is created, it can be analysed using the gprof application. However, it relies on the libc to provide helper functions ("_mcount") at linking time that musl's libc doesn't provide (https://www.openwall.com/lists/musl/2014/11/05/2). The same limitation arises for ftrace as it uses the symbols added by the "-pg" option to operate
* **Perf_events**: perf is a linux kernel utility that allows access to hardware counters which give information on the number of occurrences of a certain event. It falls in the category of "statistical profilers" which means that it is able to say which functions use the most CPU time but not give an actual reading of the time used by the function

**Demos and test suites**:

* **stress**: High speed manipulations on a GUI to observe its behaviour under load. Problem: No information returned
* **benchmark**: Does a series of low level operations and returns the number of FPS achieved for the tested configurations. Problem: Results are given in FPS, not in execution time per function. Results are displayed in GUI, maybe not in CLI
* **tests**: Unit test suite. Problem: complex build and run procedures

**Modifs to workflow**:

Github action (check_perf.yml):

* Use the commit that triggered the action instead of fixed version

Dockerfile (Dockerfile.lvgl):

* Create an argument to provide the SO3 branch that should be used (mainly for debug, default value = main)
