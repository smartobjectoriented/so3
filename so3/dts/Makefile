
dtb-$(CONFIG_VEXPRESS) += vexpress.dtb vexpress_avz.dtb
dtb-$(CONFIG_RPI4) += rpi4.dtb
dtb-$(CONFIG_RPI4_64) += rpi4_64.dtb
dtb-$(CONFIG_VIRT64) += virt64.dtb virt64_avz_pv.dtb virt64_avz_vt.dtb

ifeq ($(CONFIG_SOO),y)
ifeq ($(CONFIG_AVZ),)

ifeq ($(CONFIG_SOO_REFSO3),y)
ifeq ($(CONFIG_RAMDEV),y)
dtb-y += refso3_ramfs.dtb meso3.dtb
else
dtb-y += refso3.dtb
endif
endif

endif
endif
