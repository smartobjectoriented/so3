


ifneq ($(CONFIG_ARM64VT),y)
obj-y += hyp-stub.o
endif

obj-y += head.o exception.o traps.o

obj-y += fault.o backtrace.o 
obj-y += backtrace.o backtrace_asm.o
obj-y += cache_v8.o cache.o context.o

obj-$(CONFIG_AVZ) += hypercalls.o domain.o migration.o

obj-y += smccc-call.o

obj-y += thread.o ptrace.o 

obj-$(CONFIG_MMU) += mmu.o

obj-$(CONFIG_ARM64VT) += #smmu.o

obj-y += lib/

obj-$(CONFIG_SO3VIRT) += setup.o

obj-y += $(TARGET)/
 
