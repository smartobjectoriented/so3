
dtb-$(CONFIG_RPI4) += rpi4.dtb
dtb-$(CONFIG_RPI4_64) += rpi4_64.dtb rpi4_64_avz.dtb
dtb-$(CONFIG_VIRT64) += virt64.dtb virt64_avz.dtb virt64_capsule.dtb virt64_lvperf.dtb
dtb-$(CONFIG_VIRT32) += virt32.dtb virt32_lvperf.dtb
dtb-$(CONFIG_VERDIN_IMX8MP) += verdin-imx8mp.dtb verdin-imx8mp_avz.dtb

ifeq ($(CONFIG_SOO),y)
ifeq ($(CONFIG_AVZ),)

ifeq ($(CONFIG_SOO_REFSO3),y)
ifeq ($(CONFIG_RAMDEV),y)
dtb-y += refso3_ramfs.dtb  
else
dtb-y += refso3.dtb
endif
endif

endif
endif
