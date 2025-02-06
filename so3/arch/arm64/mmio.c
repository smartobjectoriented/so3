/*
 * Copyright (C) 2024 Daniel Rossier <daniel.rossier@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Generd2aal Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <common.h>
#include <mmio.h>
#include <types.h>

#include <device/arch/vgic.h>

#include <asm/io.h>

/*
 * These definitions are heavly borrowed from jailhouse hypervisor
 */

void mmio_perform_access(void *base, struct mmio_access *mmio)
{
	void *addr = base + mmio->address;

	if (mmio->is_write)
		switch (mmio->size) {
		case 1:
			iowrite8(addr, mmio->value);
			break;
		case 2:
			iowrite16(addr, mmio->value);
			break;
		case 4:
			iowrite32(addr, mmio->value);
			break;
#if BITS_PER_LONG == 64
		case 8:
			iowrite64(addr, mmio->value);
			break;
#endif
		}
	else
		switch (mmio->size) {
		case 1:
			mmio->value = ioread8(addr);
			break;
		case 2:
			mmio->value = ioread16(addr);
			break;
		case 4:
			mmio->value = ioread32(addr);
			break;
#if BITS_PER_LONG == 64
		case 8:
			mmio->value = ioread64(addr);
			break;
#endif
		}
}

/**
 * Dispatch MMIO access of a CPU.
 * @param mmio	MMIO access description. @a mmio->value will receive the
 * 		result of a successful read access. All @a mmio fields
 * 		may have been modified on return.
 *
 * @return MMIO_HANDLED on success, MMIO_UNHANDLED if no region is registered
 *         for the access address and size, or MMIO_ERROR if an access error was detected.
 *
 */
enum mmio_result mmio_handle_access(struct mmio_access *mmio)
{
	/* Currently, only GIC access is handled via a data abort exception */
	mmio->address -= (addr_t)gic->gicd_paddr;

	return gic_handle_dist_access(mmio);
}

/* AARCH64_TODO: we can use SXTB, SXTH, SXTW */
/* Extend the value of 'size' bits to a signed long */
static inline unsigned long sign_extend(unsigned long val, unsigned int size)
{
	unsigned long mask = 1UL << (size - 1);

	return (val ^ mask) - mask;
}

int mmio_dabt_decode(cpu_regs_t *regs, unsigned long esr)
{
	enum mmio_result mmio_result;
	struct mmio_access mmio;
	unsigned long hpfar;
	unsigned long hdfar;
	u64 *__regs = (u64 *)regs;

	/* Decode the syndrome fields */
	u32 iss = ESR_ISS(esr);
	u32 isv = iss >> 24;
	u32 sas = iss >> 22 & 0x3;
	u32 sse = iss >> 21 & 0x1;

	/* Syndrom Register Transfer */
	u32 srt = iss >> 16 & 0x1f;

	u32 ea = iss >> 9 & 0x1;
	u32 cm = iss >> 8 & 0x1;
	u32 s1ptw = iss >> 7 & 0x1;
	u32 is_write = iss >> 6 & 0x1;
	u32 size = 1 << sas;

	hpfar = read_sysreg(hpfar_el2);
	hdfar = read_sysreg(far_el2);

	mmio.address = hpfar << 8;
	mmio.address |= hdfar & 0xfff;
	/*
         * Invalid instruction syndrome means multiple access or writeback,
         * there is nothing we can do.
         */
	if (!isv)
		goto error_unhandled;

	/* Re-inject abort during page walk, cache maintenance or external */
	if (s1ptw || ea || cm) {
		panic("Not handled\n");
	}

	if (is_write) {
		/* Load the value to write from the src register */
		mmio.value = (srt == 31) ? 0 : __regs[srt];
		if (sse && size < sizeof(unsigned long))
			mmio.value = sign_extend(mmio.value, 8 * size);
	} else {
		mmio.value = 0;
	}

	mmio.is_write = is_write;
	mmio.size = size;

	mmio_result = mmio_handle_access(&mmio);
	if (mmio_result == MMIO_ERROR)
		return TRAP_FORBIDDEN;

	if (mmio_result == MMIO_UNHANDLED)
		goto error_unhandled;

	/* Put the read value into the dest register */
	if (!is_write && (srt != 31)) {
		if (sse && size < sizeof(unsigned long))
			mmio.value = sign_extend(mmio.value, 8 * size);
		__regs[srt] = mmio.value;
	}

	/* Skip instruction */
	regs->pc += ESR_IL(esr) ? 4 : 2;

	return TRAP_HANDLED;

error_unhandled:
	panic("Unhandled data %s at 0x%lx(%d)\n", (is_write ? "write" : "read"),
	      mmio.address, size);

	return TRAP_UNHANDLED;
}