#
# Makefile for the linux kernel.
#
#

obj-y 	+= setup.o
obj-y	+= domcall.o
obj-y	+= evtchn.o
obj-y	+= gnttab.o
obj-y   += soo_guest_activity.o
obj-y   += soo_id.o
obj-y   += xmlui.o
obj-y	+= vbus/ vbstore/ console/ debug/ 

# dependencies on generated files need to be listed explicitly

mrproper clean:
	@find . -name "*.o" | xargs rm -f


