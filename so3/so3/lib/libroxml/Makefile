lib-y += roxml_buff.o roxml_commit.o roxml_content.o 
lib-y += roxml_core.o roxml_edit.o roxml_mem.o roxml_nav.o
lib-y += roxml_parser.o roxml_xpath.o roxml_file.o


# The XPath evaluation computes in doubles.

ifeq ($(CONFIG_ARCH_ARM64),y)
CFLAGS_REMOVE_roxml_xpath.o = -mgeneral-regs-only
endif
