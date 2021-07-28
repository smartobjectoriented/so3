/*
 * Copyright (C) 2021 	   Nicolas Müller <nicolas.muller1@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <common.h>
#include <asm/processor.h>
#include <asm/fault.h>

void __instr_addr_misalignment(void) {
    lprintk("### Instruction address misaligned ###\n");
	kernel_panic();
}

void __instr_access_fault(void) {
    lprintk("### Instruction access fault ###\n");
	kernel_panic();
}

void __illegal_instr(void) {
    lprintk("### Illegal instruction ###\n");
	kernel_panic();
}

void __load_addr_misalignement(void) {
    lprintk("### Load address misaligned !! ###\n");
	kernel_panic();
}

void __load_access_fault(void) {
    lprintk("### Load access fault ###\n");
	kernel_panic();
}

void __store_AMO_addr_misaligned(void) {
    lprintk("### Store/AMO address misaligned ###\n");
	kernel_panic();
}

void __store_AMO_access_fault(void) {
    lprintk("### Store/AMO access fault ###\n");
	kernel_panic();
}

void __instr_page_fault(void) {
    lprintk("### Instruction page fault ###\n");
	kernel_panic();
}

void __load_page_fault(void) {
    lprintk("### Load page fault ###\n");
	kernel_panic();
}

void __store_AMO_page_fault(void) {
    lprintk("### Store/AMO page fault ###\n");
	kernel_panic();
}

void kernel_panic(void)
{
	lprintk("%s: entering infinite loop...\n", __func__);

	/* Stop all activities. */
	local_irq_disable();

	while (1);
}

void _bug(char *file, int line)
{
	lprintk("BUG in %s at line: %d\n", file, line);

	kernel_panic();
}
