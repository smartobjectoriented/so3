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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef EVTCHN_H
#define EVTCHN_H

#include <common.h>
#include <smp.h>
#include <softirq.h>

#include <avz/sched.h>

/*
 * send_guest_vcpu_virq: Notify guest via a per-VCPU VIRQ.
 *  @v:        VCPU to which virtual IRQ should be sent
 *  @virq:     Virtual IRQ number (VIRQ_*)
 */
void send_guest_virq(struct domain *d, int virq);

/* Send a notification from a given domain's event-channel port. */
void evtchn_send(struct domain *d, unsigned int lport);

/* Bind a local event-channel port to the specified VCPU. */
long evtchn_bind_vcpu(unsigned int port, unsigned int vcpu_id);

void do_event_channel_op(avz_hyp_t *args);

void event_channel_init(void);

void evtchn_bind_existing_interdomain(struct domain *ld, struct domain *remote,
				      int levtchn, int revtchn);

#endif /* EVTCHN_H */
