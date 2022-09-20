.. doc Documentation master file.

.. image:: img/heigvd-reds.png
   :align: right
   :width: 180px
   :height: 70px
   :target: http://reds.heig-vd.ch/en/rad
   
.. image:: img/SO3-Logo.png
   :align: center
   :width: 200px
   :height: 150px

.. toctree::
   :maxdepth: 4
   :numbered:
   :hidden:
  
   introduction
   architecture
   user_guide
   so3_jtag_rpi4
   lvgl
   lwip
   
============================================
Smart Object Oriented (SO3) Operating System
============================================

Smart Object Oriented Operating system is a compact, lightweight, full featured and extensible
operating system particularly well-suited for embedded systems in general.

The documentation is constantly evolving over the time and further details will be
available soon.

SO3 Concepts and Architecture
=============================

- :ref:`Introduction to SO3 <introduction>`
- :ref:`Architecture <architecture>`

Setup and Environment
=====================

-  :ref:`User Guide <user_guide>`
-  :ref:`Debugging with JTAG on Raspberry Pi 4 <so3_jtag_rpi4>`

  
Development flow
================

The master contains the last released version of the SOO framework.

.. important::

   It is not allowed to push directly to the master. Please do a merge
   request as soon as your development is stable.
   
If you want to contribute, please first contact `the maintainer <SOO_mail_>`__ and explain your motivation so that
you can be granted as developer. 
Each development leads to a new issue with its related branch. You can develop freely, add comments along the issue
and perform a merge request as soon as your development gets stable enough. A review will be done and your contributions
will be merged in the master branch. 



Discussion forum
================

A `dedicated discussion forum <https://discourse.heig-vd.ch/c/so3>`__
is available for all questions/remarks/suggestions related to SO3.
Do not hesitate to create topics and to contribute.
     
.. _SOO_mail: info@soo.tech
