
/*
 * RAMFB driver
 *
 * Copyright (C) 2022 Adrian Negreanu
 * Copyright (C) 2023 Daniel Rossier, daniel.rossier@heig-vd.ch
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define pr_fmt(fmt) "ramfb: " fmt

#include <common.h>
#include <heap.h>
#include <memory.h>
#include <errno.h>

#include <asm/mmu.h>

#include <device/device.h>
#include <device/driver.h>

#define IOCTL_HRES 1
#define IOCTL_VRES 2
#define IOCTL_SIZE 3

#define RAMFB_DRIVER_VIDEO_WIDTH 	128
#define RAMFB_DRIVER_VIDEO_HEIGHT	128

#define RAMFB_DRIVER_VIDEO_FORMAT_RGB565

#define PACKED                  __attribute__((packed))
#define QFW_CFG_FILE_DIR        0x19
#define QFW_CFG_INVALID         0xffff

/* fw_cfg DMA commands */
#define QFW_CFG_DMA_CTL_ERROR   0x01
#define QFW_CFG_DMA_CTL_READ    0x02
#define QFW_CFG_DMA_CTL_SKIP    0x04
#define QFW_CFG_DMA_CTL_SELECT  0x08
#define QFW_CFG_DMA_CTL_WRITE   0x10

#define QFW_CFG_OFFSET_DATA8     0     /* Data Register address: Base + 0 (8 bytes). */
#define QFW_CFG_OFFSET_DATA16    0
#define QFW_CFG_OFFSET_DATA32    0
#define QFW_CFG_OFFSET_DATA64    0
#define QFW_CFG_OFFSET_SELECTOR  8     /* Selector Register address: Base + 8. */
#define QFW_CFG_OFFSET_DMA64     16    /* DMA Address address: Base + 16 (8 bytes). */
#define QFW_CFG_OFFSET_DMA32     20    /* DMA Address address: Base + 16 (8 bytes). */

#define fourcc_code(a, b, c, d) ((u32)(a) | ((u32)(b) << 8) | \
		((u32)(c) << 16) | ((u32)(d) << 24))

#define DRM_FORMAT_RGB565       fourcc_code('R', 'G', '1', '6') /* [15:0] R:G:B 5:6:5 little endian */
#define DRM_FORMAT_RGB888       fourcc_code('R', 'G', '2', '4') /* [23:0] R:G:B little endian */
#define DRM_FORMAT_XRGB8888     fourcc_code('X', 'R', '2', '4') /* [31:0] x:R:G:B 8:8:8:8 little endian */

/* this field is calculated by register_framebuffer()
 * but register_framebuffer() can't be called before ramfb_alloc()
 * so set line_length to zero.
 */

struct ramfb_mode {
	const char *format;
	u32 drm_format;
	u32 bpp;
};

typedef struct {
	unsigned long screen_size;
	uint8_t *screen_buffer;

	struct ramfb_mode mode;

	uint32_t xres;
	uint32_t yres;

	uint32_t stride; /* line length */
} ramfb_t;

union FwCfgSigRead {
    uint32_t theInt;
    char bytes[sizeof(int)];
};

struct qfw_cfg_etc_ramfb {
	u64 addr;
	u32 fourcc;
	u32 flags;
	u32 width;
	u32 height;
	u32 stride;
} PACKED;

struct qfw_cfg_file {
	u32  size;
	u16  select;
	u16  reserved;
	char name[56];
} PACKED;

typedef struct qfw_cfg_dma {
    uint32_t control;
    uint32_t length;
    uint64_t address;
} __attribute__((__packed__)) qfw_cfg_dma;

static const struct ramfb_mode fb_mode = {
#if defined(RAMFB_DRIVER_VIDEO_FORMAT_RGB565)
	.format	= "r5g6b5",
	.drm_format = DRM_FORMAT_RGB565,
	.bpp	= 16,
#elif defined(RAMFB_DRIVER_VIDEO_FORMAT_RGB888)
	.format	= "r8g8b8",
	.drm_format = DRM_FORMAT_RGB888,
	.bpp	= 24,
#elif defined(RAMFB_DRIVER_VIDEO_FORMAT_XRGB8888)
	.format	= "x8r8g8b8",
	.drm_format = DRM_FORMAT_XRGB8888,
	.bpp = 32,
#endif
};

/* Temporarly */
ramfb_t *__fbi;

static void mmio_write16(void *addr, uint16_t val) {
    volatile uint16_t *mmio_w = (volatile uint16_t *) addr;

    *mmio_w = val;
}

static uint64_t mmio_read_bsw64(void *addr) {
    return __builtin_bswap64(*((volatile uint64_t *) addr));
}

static void mmio_write_bsw64(void *addr, uint64_t val) {
    volatile uint64_t *mmio_w = (volatile uint64_t *) addr;

    *mmio_w = __builtin_bswap64(val);
}

static void qfw_cfg_dma_transfer(void *fw_cfg_base, void *address, uint32_t length, uint32_t control) {
	volatile qfw_cfg_dma *access;
	void *__address;
	struct qfw_cfg_etc_ramfb *etc_ramfb;

	if (length == 0)
		return ;

	access = malloc(sizeof(qfw_cfg_dma));
	BUG_ON(!access);

	__address = malloc(length);
	BUG_ON(!__address);

	memcpy(__address, address, length);

	access->address = __builtin_bswap64((uint64_t) __pa(__address));
	access->length = __builtin_bswap32(length);
	access->control = __builtin_bswap32(control);

	barrier();

	etc_ramfb = (struct qfw_cfg_etc_ramfb *) __address;

	mmio_write_bsw64(fw_cfg_base + 0x10, (uint32_t) __pa(access));

	while (__builtin_bswap32(access->control) & ~QFW_CFG_DMA_CTL_ERROR) { }

	memcpy(address, __address, length);

	barrier();

}

static void qfw_cfg_read(void *fw_cfg_base, void *buf, int len) {
	qfw_cfg_dma_transfer(fw_cfg_base, buf, len, QFW_CFG_DMA_CTL_READ);
}

static void qfw_cfg_read_entry(void  *fw_cfg_base, void *buf, int e, int len) {
	uint32_t control = (e << 16) | QFW_CFG_DMA_CTL_SELECT | QFW_CFG_DMA_CTL_READ;

	qfw_cfg_dma_transfer(fw_cfg_base, buf, len, control);
}

void qfw_cfg_write_entry(void *fw_cfg_base, void *buf, uint32_t e, uint32_t len) {
	uint32_t control = (e << 16) | QFW_CFG_DMA_CTL_SELECT | QFW_CFG_DMA_CTL_WRITE;

	qfw_cfg_dma_transfer(fw_cfg_base, buf, len, control);
}

static int qfw_cfg_find_file(void *fw_cfg_base, const char *filename)
{
	uint32_t count, e, select;

	qfw_cfg_read_entry(fw_cfg_base, &count, QFW_CFG_FILE_DIR, sizeof(count));
	count = __builtin_bswap32(count);

	for (select = 0, e = 0; e < count; e++) {
		struct qfw_cfg_file qfile;
		qfw_cfg_read(fw_cfg_base, &qfile, sizeof(qfile));
		if (strcmp(qfile.name, filename) == 0)
			select = __builtin_bswap16(qfile.select);
	}
	return select;
}

static int ramfb_alloc(void *fw_cfg_base, ramfb_t *fbi)
{
	u32 select;
	struct qfw_cfg_etc_ramfb etc_ramfb;

	select = qfw_cfg_find_file(fw_cfg_base, "etc/ramfb");
	if (select == 0) {
		printk("QEMU-ramfb: fw_cfg (etc/ramfb) file not found\n");
		return -1;
	}
	printk("QEMU-ramfb: fw_cfg (etc/ramfb) file at slot 0x%x\n", select);

	fbi->screen_size = RAMFB_DRIVER_VIDEO_WIDTH * RAMFB_DRIVER_VIDEO_HEIGHT * (fbi->mode.bpp / 8);

	fbi->screen_buffer = (void *) get_contig_free_vpages(fbi->screen_size / PAGE_SIZE);

	if (!fbi->screen_buffer) {
		printk("QEMU-ramfb: Unable to use FB\n");

		return -1;
	}

	etc_ramfb.addr = (uintptr_t) __pa(fbi->screen_buffer);

	etc_ramfb.addr = __builtin_bswap64(etc_ramfb.addr);
	etc_ramfb.fourcc = __builtin_bswap32(fb_mode.drm_format);
	etc_ramfb.flags  = __builtin_bswap32(0);
	etc_ramfb.width  = __builtin_bswap32(fbi->xres);
	etc_ramfb.height = __builtin_bswap32(fbi->yres);
	etc_ramfb.stride = __builtin_bswap32(fbi->stride);

	qfw_cfg_write_entry(fw_cfg_base, &etc_ramfb, select, sizeof(etc_ramfb));

	return 0;
}

int check_fw_cfg_dma(void *fw_cfg_base) {
    union FwCfgSigRead cfg_sig_read;

    mmio_write16(fw_cfg_base + 0x8, 0x0000);

    cfg_sig_read.theInt = *((volatile uint32_t *)(fw_cfg_base));

    if (cfg_sig_read.bytes[0] == 'Q' && cfg_sig_read.bytes[1] == 'E' && cfg_sig_read.bytes[2] == 'M' && cfg_sig_read.bytes[3] == 'U') {
        if (mmio_read_bsw64(fw_cfg_base + 0x10) == 0x51454d5520434647) {
            return 1;
        }
    }    return 0;
}

void *fb_mmap(int fd, addr_t virt_addr, uint32_t page_count)
{
	uint32_t i, page;
	pcb_t *pcb = current()->pcb;

#if 0
	virt_addr = malloc(page_count*PAGE_SIZE);
	BUG_ON(!virt_addr);
#endif

#if 1
	for (i = 0; i < page_count; i++) {
		/* Map a process' virtual page to the physical one (here the VRAM). */
		page = (addr_t) __pa(__fbi->screen_buffer + i * PAGE_SIZE);

		create_mapping(pcb->pgtable, virt_addr + (i * PAGE_SIZE), page, PAGE_SIZE, false);
	}
#endif

	return (void *) virt_addr;
}


int fb_ioctl(int fd, unsigned long cmd, unsigned long args)
{
	switch (cmd) {

	case IOCTL_HRES:
		*((uint32_t *) args) = RAMFB_DRIVER_VIDEO_WIDTH;
		return 0;

	case IOCTL_VRES:
		*((uint32_t *) args) = RAMFB_DRIVER_VIDEO_HEIGHT;
		return 0;

	case IOCTL_SIZE:
		*((uint32_t *) args) = RAMFB_DRIVER_VIDEO_HEIGHT * RAMFB_DRIVER_VIDEO_WIDTH * __fbi->mode.bpp/8; /* assume 32bpp */
		return 0;

	default:
		/* Unknown command. */
		return -1;
	}
}

struct file_operations ramfb_fops = {
	.mmap = fb_mmap,
	.ioctl = fb_ioctl
};

struct devclass ramfb_cdev = {
	.class = DEV_CLASS_FB,
	.type = VFS_TYPE_DEV_FB,
	.fops = &ramfb_fops,
};

static int ramfb_init(dev_t *dev, int fdt_offset)
{
	int ret;
	ramfb_t *fbi;
	const struct fdt_property *prop;
	int prop_len;

	void *fw_cfg_base;		/* base address of the registers */

	ret = -ENODEV;

	fbi = malloc(sizeof(ramfb_t));
	BUG_ON(!fbi);

	__fbi = fbi;

	prop = fdt_get_property(__fdt_addr, fdt_offset, "reg", &prop_len);
	BUG_ON(!prop);

	BUG_ON(prop_len != 2 * sizeof(unsigned long));

	#ifdef CONFIG_ARCH_ARM32
		fw_cfg_base = (void *) io_map(fdt32_to_cpu(((const fdt32_t *) prop->data)[0]), fdt32_to_cpu(((const fdt32_t *) prop->data)[1]));
	#else
		fw_cfg_base = (void *) io_map(fdt64_to_cpu(((const fdt64_t *) prop->data)[0]), fdt64_to_cpu(((const fdt64_t *) prop->data)[1]));
	#endif

	if (check_fw_cfg_dma(fw_cfg_base)) {
	    printk("guest fw_cfg dma-interface enabled \n");
	  } else {
	    printk("guest fw_cfg dma-interface NOT enabled - abort \n");
	    return -1;
	  }

	fbi->mode.format = fb_mode.format;
	fbi->mode = fb_mode;

	fbi->xres = RAMFB_DRIVER_VIDEO_WIDTH;
	fbi->yres = RAMFB_DRIVER_VIDEO_HEIGHT;

	/* this field is calculated by register_framebuffer()
	 * but register_framebuffer() can't be called before ramfb_alloc()
	 * so set line_length to zero.
	 */
	fbi->stride = (fbi->mode.bpp/8) * fbi->xres;

	ret = ramfb_alloc(fw_cfg_base, fbi);
	if (ret < 0) {
		printk("QEMU-ramfb: Unable to allocate ramfb: %d\n", ret);
		BUG();
	}

	/* Register the framebuffer so it can be accessed from user space. */
	devclass_register(dev, &ramfb_cdev);

	printk("QEMU-ramfb: size %lu registered\n", fbi->screen_size);

	return 0;
}

REGISTER_DRIVER_POSTCORE("qemu,fw-cfg-mmio", ramfb_init);

