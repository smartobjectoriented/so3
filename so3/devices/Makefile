obj-y = fdt.o device.o irq.o timer.o console.o

obj-$(CONFIG_UART) += serial.o
obj-$(CONFIG_I2C) += i2c.o

obj-$(CONFIG_MMC) += mmc/
obj-$(CONFIG_UART) += serial/
obj-$(CONFIG_I2C) += i2c/
obj-$(CONFIG_RPI_SENSE) += rpisense/

obj-$(CONFIG_RAMDEV) += ramdev/

obj-y += timer/ irq/

obj-y += mydev.o
