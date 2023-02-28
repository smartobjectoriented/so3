

/**
 * Currently, this file contains some code which can be used anywhere
 * to perform stress test and general testing.
 *
 * All this code will be consolidated in some future...
 **/

static int alphabet_fn(void *arg) {
	int res;
	unsigned int evtchn;
	struct evtchn_alloc_unbound alloc_unbound;

	printk("Alphabet roundtrip...\n");

#if 0
	set_timer(&timer, NOW() + SECONDS(10));
#endif

	/* Allocate an event channel associated to the ring */
	alloc_unbound.remote_dom = 0;
	alloc_unbound.dom = DOMID_SELF;

	hypercall_trampoline(__HYPERVISOR_event_channel_op, EVTCHNOP_alloc_unbound, (long) &alloc_unbound, 0, 0);
	evtchn = alloc_unbound.evtchn;
	lprintk("## evtchn got from avz: %d\n", evtchn);

	res = bind_evtchn_to_irq_handler(evtchn, evt_interrupt, NULL, NULL);

	do_sync_dom(0, DC_PRE_SUSPEND);

	while (1) {

		/* printk("### heap size: %x\n", heap_size()); */
		msleep(500);

		/* Simply display the current letter which is incremented each time a ME comes back */
		lprintk("(%d)",  ME_domID());
		//printk("%c ", *((char *) localinfo_data));
		lprintk("X ");
	}

	return 0;
}
