/*
 * Copyright (C) 2016-2020 Daniel Rossier <daniel.rossier@soo.tech>
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

#ifndef BACKTRACE_H
#define BACKTRACE_H

void dump_stack(void);

void show_registers(cpu_regs_t *regs);

void dump_execution_state(void);
void dump_all_execution_state(void);

static inline void show_execution_state(cpu_regs_t *regs)
{
    show_registers(regs);
}

#endif /* BACKTRACE_H */

