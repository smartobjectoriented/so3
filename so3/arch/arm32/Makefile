
obj-y += head.o
obj-y += setup.o exception.o context.o fault.o
obj-y += cache_v7_asm.o cache_v7.o cache-cp15.o
obj-y += thread.o ptrace.o
obj-y += vfp.o
obj-y += backtrace.o backtrace_asm.o

obj-y += lib/

obj-$(CONFIG_MMU) += mmu.o

obj-$(CONFIG_AVZ) += hypercalls.o domain.o migration.o

#obj-$(CONFIG_PSCI) += smccc-call.o

obj-y += $(TARGET)/

