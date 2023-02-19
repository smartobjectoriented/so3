
# Object file lists.

obj-y += 	main.o \
		eabi_compat.o \
		delay.o \
		calibrate.o \
		thread.o \
		schedule.o \
		mutex.o  \
		spinlock.o \
		syscalls.o \
		softirq.o \
		timer.o 

obj-$(CONFIG_CPU_PSCI) += psci_smp.o
obj-$(CONFIG_CPU_SPIN_TABLE) += spin_table.o

obj-$(CONFIG_SMP) += smp.o 
obj-y += timer.o
obj-y += bitmap.o

obj-y += softirq.o
obj-y += spinlock.o

obj-$(CONFIG_AVZ) += avz/

obj-$(CONFIG_MMU) += process.o ptrace.o

EXTRA_CFLAGS += -I$(srctree)/include/net

