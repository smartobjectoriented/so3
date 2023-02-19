/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2005
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 ********************************************************************
 * NOTE: This header file defines an interface to U-Boot. Including
 * this (unmodified) header file in another file is considered normal
 * use of U-Boot, and does *not* fall under the heading of "derived
 * work".
 ********************************************************************
 */

#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <types.h>

#include <libfdt/libfdt.h>
#include <libfdt/fdt_support.h>

#define FIT_IMAGES_PATH		"/images"
#define FIT_CONFS_PATH		"/configurations"

/* hash/signature node */
#define FIT_HASH_NODENAME	"hash"
#define FIT_ALGO_PROP		"algo"
#define FIT_VALUE_PROP		"value"
#define FIT_IGNORE_PROP		"uboot-ignore"
#define FIT_SIG_NODENAME	"signature"

/* image node */
#define FIT_DATA_PROP		"data"
#define FIT_DATA_POSITION_PROP	"data-position"
#define FIT_DATA_OFFSET_PROP	"data-offset"
#define FIT_DATA_SIZE_PROP	"data-size"
#define FIT_TIMESTAMP_PROP	"timestamp"
#define FIT_DESC_PROP		"description"
#define FIT_ARCH_PROP		"arch"
#define FIT_TYPE_PROP		"type"
#define FIT_OS_PROP		"os"
#define FIT_COMP_PROP		"compression"
#define FIT_ENTRY_PROP		"entry"
#define FIT_LOAD_PROP		"load"

/* configuration node */
#define FIT_KERNEL_PROP		"kernel"
#define FIT_RAMDISK_PROP	"ramdisk"
#define FIT_FDT_PROP		"fdt"
#define FIT_LOADABLE_PROP	"loadables"
#define FIT_DEFAULT_PROP	"default"
#define FIT_SETUP_PROP		"setup"
#define FIT_FPGA_PROP		"fpga"
#define FIT_FIRMWARE_PROP	"firmware"
#define FIT_STANDALONE_PROP	"standalone"

#define FIT_MAX_HASH_LEN	HASH_MAX_DIGEST_SIZE

enum ih_category {
	IH_ARCH,
	IH_COMP,
	IH_OS,
	IH_TYPE,

	IH_COUNT,
};

/*
 * Operating System Codes
 *
 * The following are exposed to uImage header.
 * Do not change values for backward compatibility.
 */
enum {
	IH_OS_INVALID		= 0,	/* Invalid OS	*/
	IH_OS_OPENBSD,			/* OpenBSD	*/
	IH_OS_NETBSD,			/* NetBSD	*/
	IH_OS_FREEBSD,			/* FreeBSD	*/
	IH_OS_4_4BSD,			/* 4.4BSD	*/
	IH_OS_LINUX,			/* Linux	*/
	IH_OS_SVR4,			/* SVR4		*/
	IH_OS_ESIX,			/* Esix		*/
	IH_OS_SOLARIS,			/* Solaris	*/
	IH_OS_IRIX,			/* Irix		*/
	IH_OS_SCO,			/* SCO		*/
	IH_OS_DELL,			/* Dell		*/
	IH_OS_NCR,			/* NCR		*/
	IH_OS_LYNXOS,			/* LynxOS	*/
	IH_OS_VXWORKS,			/* VxWorks	*/
	IH_OS_PSOS,			/* pSOS		*/
	IH_OS_QNX,			/* QNX		*/
	IH_OS_U_BOOT,			/* Firmware	*/
	IH_OS_RTEMS,			/* RTEMS	*/
	IH_OS_ARTOS,			/* ARTOS	*/
	IH_OS_UNITY,			/* Unity OS	*/
	IH_OS_INTEGRITY,		/* INTEGRITY	*/
	IH_OS_OSE,			/* OSE		*/
	IH_OS_PLAN9,			/* Plan 9	*/
	IH_OS_OPENRTOS,		/* OpenRTOS	*/
	IH_OS_ARM_TRUSTED_FIRMWARE,     /* ARM Trusted Firmware */
	IH_OS_TEE,			/* Trusted Execution Environment */

	IH_OS_COUNT,
};

/*
 * CPU Architecture Codes (supported by Linux)
 *
 * The following are exposed to uImage header.
 * Do not change values for backward compatibility.
 */
enum {
	IH_ARCH_INVALID		= 0,	/* Invalid CPU	*/
	IH_ARCH_ALPHA,			/* Alpha	*/
	IH_ARCH_ARM,			/* ARM		*/
	IH_ARCH_I386,			/* Intel x86	*/
	IH_ARCH_IA64,			/* IA64		*/
	IH_ARCH_MIPS,			/* MIPS		*/
	IH_ARCH_MIPS64,			/* MIPS	 64 Bit */
	IH_ARCH_PPC,			/* PowerPC	*/
	IH_ARCH_S390,			/* IBM S390	*/
	IH_ARCH_SH,			/* SuperH	*/
	IH_ARCH_SPARC,			/* Sparc	*/
	IH_ARCH_SPARC64,		/* Sparc 64 Bit */
	IH_ARCH_M68K,			/* M68K		*/
	IH_ARCH_NIOS,			/* Nios-32	*/
	IH_ARCH_MICROBLAZE,		/* MicroBlaze   */
	IH_ARCH_NIOS2,			/* Nios-II	*/
	IH_ARCH_BLACKFIN,		/* Blackfin	*/
	IH_ARCH_AVR32,			/* AVR32	*/
	IH_ARCH_ST200,			/* STMicroelectronics ST200  */
	IH_ARCH_SANDBOX,		/* Sandbox architecture (test only) */
	IH_ARCH_NDS32,			/* ANDES Technology - NDS32  */
	IH_ARCH_OPENRISC,		/* OpenRISC 1000  */
	IH_ARCH_ARM64,			/* ARM64	*/
	IH_ARCH_ARC,			/* Synopsys DesignWare ARC */
	IH_ARCH_X86_64,			/* AMD x86_64, Intel and Via */
	IH_ARCH_XTENSA,			/* Xtensa	*/
	IH_ARCH_RISCV,			/* RISC-V */

	IH_ARCH_COUNT,
};

/*
 * Image Types
 *
 * "Standalone Programs" are directly runnable in the environment
 *	provided by U-Boot; it is expected that (if they behave
 *	well) you can continue to work in U-Boot after return from
 *	the Standalone Program.
 * "OS Kernel Images" are usually images of some Embedded OS which
 *	will take over control completely. Usually these programs
 *	will install their own set of exception handlers, device
 *	drivers, set up the MMU, etc. - this means, that you cannot
 *	expect to re-enter U-Boot except by resetting the CPU.
 * "RAMDisk Images" are more or less just data blocks, and their
 *	parameters (address, size) are passed to an OS kernel that is
 *	being started.
 * "Multi-File Images" contain several images, typically an OS
 *	(Linux) kernel image and one or more data images like
 *	RAMDisks. This construct is useful for instance when you want
 *	to boot over the network using BOOTP etc., where the boot
 *	server provides just a single image file, but you want to get
 *	for instance an OS kernel and a RAMDisk image.
 *
 *	"Multi-File Images" start with a list of image sizes, each
 *	image size (in bytes) specified by an "uint32_t" in network
 *	byte order. This list is terminated by an "(uint32_t)0".
 *	Immediately after the terminating 0 follow the images, one by
 *	one, all aligned on "uint32_t" boundaries (size rounded up to
 *	a multiple of 4 bytes - except for the last file).
 *
 * "Firmware Images" are binary images containing firmware (like
 *	U-Boot or FPGA images) which usually will be programmed to
 *	flash memory.
 *
 * "Script files" are command sequences that will be executed by
 *	U-Boot's command interpreter; this feature is especially
 *	useful when you configure U-Boot to use a real shell (hush)
 *	as command interpreter (=> Shell Scripts).
 *
 * The following are exposed to uImage header.
 * Do not change values for backward compatibility.
 */

enum {
	IH_TYPE_INVALID		= 0,	/* Invalid Image		*/
	IH_TYPE_STANDALONE,		/* Standalone Program		*/
	IH_TYPE_KERNEL,			/* OS Kernel Image		*/
	IH_TYPE_RAMDISK,		/* RAMDisk Image		*/
	IH_TYPE_MULTI,			/* Multi-File Image		*/
	IH_TYPE_FIRMWARE,		/* Firmware Image		*/
	IH_TYPE_SCRIPT,			/* Script file			*/
	IH_TYPE_FILESYSTEM,		/* Filesystem Image (any type)	*/
	IH_TYPE_FLATDT,			/* Binary Flat Device Tree Blob	*/
	IH_TYPE_KWBIMAGE,		/* Kirkwood Boot Image		*/
	IH_TYPE_IMXIMAGE,		/* Freescale IMXBoot Image	*/
	IH_TYPE_UBLIMAGE,		/* Davinci UBL Image		*/
	IH_TYPE_OMAPIMAGE,		/* TI OMAP Config Header Image	*/
	IH_TYPE_AISIMAGE,		/* TI Davinci AIS Image		*/
	/* OS Kernel Image, can run from any load address */
	IH_TYPE_KERNEL_NOLOAD,
	IH_TYPE_PBLIMAGE,		/* Freescale PBL Boot Image	*/
	IH_TYPE_MXSIMAGE,		/* Freescale MXSBoot Image	*/
	IH_TYPE_GPIMAGE,		/* TI Keystone GPHeader Image	*/
	IH_TYPE_ATMELIMAGE,		/* ATMEL ROM bootable Image	*/
	IH_TYPE_SOCFPGAIMAGE,		/* Altera SOCFPGA CV/AV Preloader */
	IH_TYPE_X86_SETUP,		/* x86 setup.bin Image		*/
	IH_TYPE_LPC32XXIMAGE,		/* x86 setup.bin Image		*/
	IH_TYPE_LOADABLE,		/* A list of typeless images	*/
	IH_TYPE_RKIMAGE,		/* Rockchip Boot Image		*/
	IH_TYPE_RKSD,			/* Rockchip SD card		*/
	IH_TYPE_RKSPI,			/* Rockchip SPI image		*/
	IH_TYPE_ZYNQIMAGE,		/* Xilinx Zynq Boot Image */
	IH_TYPE_ZYNQMPIMAGE,		/* Xilinx ZynqMP Boot Image */
	IH_TYPE_ZYNQMPBIF,		/* Xilinx ZynqMP Boot Image (bif) */
	IH_TYPE_FPGA,			/* FPGA Image */
	IH_TYPE_VYBRIDIMAGE,	/* VYBRID .vyb Image */
	IH_TYPE_TEE,            /* Trusted Execution Environment OS Image */
	IH_TYPE_FIRMWARE_IVT,		/* Firmware Image with HABv4 IVT */
	IH_TYPE_PMMC,            /* TI Power Management Micro-Controller Firmware */
	IH_TYPE_STM32IMAGE,		/* STMicroelectronics STM32 Image */
	IH_TYPE_SOCFPGAIMAGE_V1,	/* Altera SOCFPGA A10 Preloader	*/

	IH_TYPE_COUNT,			/* Number of image types */
};

/*
 * Compression Types
 *
 * The following are exposed to uImage header.
 * Do not change values for backward compatibility.
 */
enum {
	IH_COMP_NONE		= 0,	/*  No	 Compression Used	*/
	IH_COMP_GZIP,			/* gzip	 Compression Used	*/
	IH_COMP_BZIP2,			/* bzip2 Compression Used	*/
	IH_COMP_LZMA,			/* lzma  Compression Used	*/
	IH_COMP_LZO,			/* lzo   Compression Used	*/
	IH_COMP_LZ4,			/* lz4   Compression Used	*/

	IH_COMP_COUNT,
};

#define IH_MAGIC	0x27051956	/* Image Magic Number		*/
#define IH_NMLEN		32	/* Image Name Length		*/

#define IMAGE_ENABLE_TIMESTAMP 0

/* cmdline argument format parsing */
int fit_parse_conf(const char *spec, ulong addr_curr,
		ulong *addr, const char **conf_name);
int fit_parse_subimage(const char *spec, ulong addr_curr,
		ulong *addr, const char **image_name);

int fit_get_subimage_count(const void *fit, int images_noffset);
void fit_print_contents(const void *fit);
void fit_image_print(const void *fit, int noffset, const char *p);

/**
 * fit_get_end - get FIT image size
 * @fit: pointer to the FIT format image header
 *
 * returns:
 *     size of the FIT image (blob) in memory
 */
static inline ulong fit_get_size(const void *fit)
{
	return fdt_totalsize(fit);
}

/**
 * fit_get_end - get FIT image end
 * @fit: pointer to the FIT format image header
 *
 * returns:
 *     end address of the FIT image (blob) in memory
 */
ulong fit_get_end(const void *fit);

/**
 * fit_get_name - get FIT node name
 * @fit: pointer to the FIT format image header
 *
 * returns:
 *     NULL, on error
 *     pointer to node name, on success
 */
static inline const char *fit_get_name(const void *fit_hdr,
		int noffset, int *len)
{
	return fdt_get_name(fit_hdr, noffset, len);
}

int fit_get_desc(const void *fit, int noffset, char **desc);
int fit_get_timestamp(const void *fit, int noffset, time_t *timestamp);

int fit_image_get_node(const void *fit, const char *image_uname);
int fit_image_get_os(const void *fit, int noffset, uint8_t *os);
int fit_image_get_arch(const void *fit, int noffset, uint8_t *arch);
int fit_image_get_type(const void *fit, int noffset, uint8_t *type);
int fit_image_get_comp(const void *fit, int noffset, uint8_t *comp);
int fit_image_get_load(const void *fit, int noffset, ulong *load);
int fit_image_get_entry(const void *fit, int noffset, ulong *entry);
int fit_image_get_data(const void *fit, int noffset,
				const void **data, size_t *size);
int fit_image_get_data_offset(const void *fit, int noffset, int *data_offset);
int fit_image_get_data_position(const void *fit, int noffset,
				int *data_position);
int fit_image_get_data_size(const void *fit, int noffset, int *data_size);
int fit_image_get_data_and_size(const void *fit, int noffset,
				const void **data, size_t *size);

int fit_image_hash_get_algo(const void *fit, int noffset, char **algo);
int fit_image_hash_get_value(const void *fit, int noffset, uint8_t **value,
				int *value_len);

int fit_set_timestamp(void *fit, int noffset, time_t timestamp);

/**
 * fit_add_verification_data() - add verification data to FIT image nodes
 *
 * @keydir:	Directory containing keys
 * @kwydest:	FDT blob to write public key information to
 * @fit:	Pointer to the FIT format image header
 * @comment:	Comment to add to signature nodes
 * @require_keys: Mark all keys as 'required'
 * @engine_id:	Engine to use for signing
 * @cmdname:	Command name used when reporting errors
 *
 * Adds hash values for all component images in the FIT blob.
 * Hashes are calculated for all component images which have hash subnodes
 * with algorithm property set to one of the supported hash algorithms.
 *
 * Also add signatures if signature nodes are present.
 *
 * returns
 *     0, on success
 *     libfdt error code, on failure
 */
int fit_add_verification_data(const char *keydir, void *keydest, void *fit,
			      const char *comment, int require_keys,
			      const char *engine_id, const char *cmdname);

int fit_image_verify_with_data(const void *fit, int image_noffset,
			       const void *data, size_t size);
int fit_image_verify(const void *fit, int noffset);
int fit_config_verify(const void *fit, int conf_noffset);
int fit_all_image_verify(const void *fit);
int fit_image_check_os(const void *fit, int noffset, uint8_t os);
int fit_image_check_arch(const void *fit, int noffset, uint8_t arch);
int fit_image_check_type(const void *fit, int noffset, uint8_t type);
int fit_image_check_comp(const void *fit, int noffset, uint8_t comp);
int fit_check_format(const void *fit);

int fit_conf_find_compat(const void *fit, const void *fdt);
int fit_conf_get_node(const void *fit, const char *conf_uname);

/**
 * fit_conf_get_prop_node() - Get node refered to by a configuration
 * @fit:	FIT to check
 * @noffset:	Offset of conf@xxx node to check
 * @prop_name:	Property to read from the conf node
 *
 * The conf- nodes contain references to other nodes, using properties
 * like 'kernel = "kernel"'. Given such a property name (e.g. "kernel"),
 * return the offset of the node referred to (e.g. offset of node
 * "/images/kernel".
 */
int fit_conf_get_prop_node(const void *fit, int noffset,
		const char *prop_name);

int fit_check_ramdisk(const void *fit, int os_noffset,
		uint8_t arch, int verify);

int calculate_hash(const void *data, int data_len, const char *algo,
			uint8_t *value, int *value_len);

#endif	/* __IMAGE_H__ */
