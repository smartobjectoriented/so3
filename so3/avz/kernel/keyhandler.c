/*
 * Copyright (C) 2014-2018 Daniel Rossier <daniel.rossier@heig-vd.ch>
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
#include <console.h>
#include <serial.h>
#include <softirq.h>
#include <ctype.h>

#include <avz/keyhandler.h>
#include <avz/sched.h>
#include <avz/domain.h>
#include <avz/event.h>

#include <asm/backtrace.h>
#include <asm/processor.h>

static struct keyhandler *key_table[256];

char keyhandler_scratch[1024];

void handle_keypress(unsigned char key)
{
	struct keyhandler *h;

	if ((h = key_table[key]) == NULL)
		return;

	h->fn(key);

}

void register_keyhandler(unsigned char key, struct keyhandler *handler)
{
	ASSERT(key_table[key] == NULL);
	key_table[key] = handler;
}

static void show_handlers(unsigned char key)
{
	int i;
	printk("'%c' pressed -> showing installed handlers\n", key);
	for (i = 0; i < ARRAY_SIZE(key_table); i++)
		if (key_table[i] != NULL)
			printk(" key '%c' (ascii '%02x') => %s\n",
					isprint(i) ? i : ' ', i, key_table[i]->desc);
}

static struct keyhandler show_handlers_keyhandler = {
	.fn = show_handlers,
	.desc = "show this message"
};

static void dump_registers(unsigned char key)
{
	printk("'%c' pressed -> dumping registers\n", key);

	/* Get local execution state out immediately, in case we get stuck. */
	printk("\n*** Dumping CPU%d host state: ***\n", smp_processor_id());

	dump_execution_state();
	printk("*** Dumping CPU%d guest state: ***\n", smp_processor_id());

#if 0
	if (is_idle_domain(current))
		printk("No guest context (CPU is idle).\n");
	else
		show_execution_state(&current->arch.guest_context.user_regs);
#endif
	printk("\n");
}

static struct keyhandler dump_registers_keyhandler = {
	.fn = dump_registers,
	.desc = "dump registers"
};

extern void vcpu_show_execution_state(void);
static void dump_agency_registers(unsigned char key)
{
	if (agency == NULL)
		return;

	printk("'%c' pressed -> dumping agency's registers\n", key);

	vcpu_show_execution_state();
}

static struct keyhandler dump_agency_registers_keyhandler = {
	.fn = dump_agency_registers,
	.desc = "dump agency registers"
};

static void dump_domains(unsigned char key)
{
	struct domain *d;
	u64    now = NOW();
	int i;

#define tmpstr keyhandler_scratch

	printk("'%c' pressed -> dumping domain info (now=0x%X:%08X)\n", key,
			(u32)(now>>32), (u32)now);

	for (i = 0; i < MAX_DOMAINS; i++) {
		d = domains[i];

		printk("General information for domain %u:\n", d->avz_shared->domID);

		printk("    dying=%d nr_pages=%d max_pages=%u\n", d->is_dying, d->avz_shared->nr_pages, d->max_pages);

		printk("VCPU information and callbacks for domain %u:\n", d->avz_shared->domID);

		printk("    CPU%d [has=%c] flags=%lx ",
			d->processor,
			d->is_running ? 'T':'F',
			d->pause_flags);

		printk("    %s\n", tmpstr);

	}

#undef tmpstr
}

static struct keyhandler dump_domains_keyhandler = {
	.fn = dump_domains,
	.desc = "dump domain (and guest debug) info"
};


void initialize_keytable(void)
{
    register_keyhandler('d', &dump_registers_keyhandler);
    register_keyhandler('h', &show_handlers_keyhandler);
    register_keyhandler('q', &dump_domains_keyhandler);

    register_keyhandler('0', &dump_agency_registers_keyhandler);

}
