/*
 * Copyright (C) 2018-2019 Daniel Rossier <daniel.rossier@soo.tech>
 * Copyright (C) 2018-2019 Baptiste Delporte <bonel@bonel.net>
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

#if 0
#define DEBUG
#endif

#include "common.h"

#ifdef SOOTEST_DRIVERS_DUMMY
/* Internal packet */
static unsigned char sent_packet_hash[SOOTEST_SHA1_HASH_LEN];

/* Pre-defined packets */
static char *predef_packets[SOOTEST_N_PREDEF_PACKETS];
static char *predef_packets_hashes[SOOTEST_N_PREDEF_PACKETS];

static char packet_bitmap[SOOTEST_N_PREDEF_PACKETS / 8];

static struct task_struct *cycle_thread;

static char echo_packet[VDUMMY_PACKET_SIZE];
static unsigned char echo_packet_hash[SOOTEST_SHA1_HASH_LEN];

static void prepare_predef_packets(void);
static void send_packet(void *buffer, size_t size);
void sootest_dummy_process_received_echo(char *packet);
static void sha1_hash(void *buffer, size_t size, void *hash);
static int cycle_thread_fct(void * param);
void sootest_dummy_init(void);
void sootest_dummy_start_cycle(void);

extern void vdummy_generate_request(char *buffer);

/**
 * Prepare the pre-defined packets.
 */
static void prepare_predef_packets(void) {
	char *packet;
	int i;
#ifdef DEBUG
	int j;
#endif

	for (i = 0 ; i < SOOTEST_N_PREDEF_PACKETS ; i++) {
		predef_packets[i] = (char *) kzalloc(VDUMMY_PACKET_SIZE, GFP_KERNEL);
		predef_packets_hashes[i] = (char *) kzalloc(SOOTEST_SHA1_HASH_LEN, GFP_KERNEL);
	}

	/* Packet 0: 0x00 0x01 .. 0x0f 0x00 0x01 ... */
	packet = predef_packets[0];
	for (i = 0 ; i < VDUMMY_PACKET_SIZE ; i++)
		packet[i] = i % 0x10;
	sha1_hash(packet, VDUMMY_PACKET_SIZE, predef_packets_hashes[0]);

	/* Packet 1: 0x00 0x01 .. 0xff 0x00 0x01 ... */
	packet = predef_packets[1];
	for (i = 0 ; i < VDUMMY_PACKET_SIZE ; i++)
		packet[i] = i % 0x100;
	sha1_hash(packet, VDUMMY_PACKET_SIZE, predef_packets_hashes[1]);

	/* Packet 2: 0xff 0xff ... */
	packet = predef_packets[2];
	memset(packet, 0xff, VDUMMY_PACKET_SIZE);
	sha1_hash(packet, VDUMMY_PACKET_SIZE, predef_packets_hashes[2]);

	/* Packet 3 : 0x00 0x00 ... */
	packet = predef_packets[3];
	memset(packet, 0, VDUMMY_PACKET_SIZE);
	sha1_hash(packet, VDUMMY_PACKET_SIZE, predef_packets_hashes[3]);

	/* Packet 4: 0x00 0xff 0x00 0xff ... */
	packet = predef_packets[4];
	for (i = 0 ; i < VDUMMY_PACKET_SIZE / 2 ; i++) {
		packet[2 * i] = 0;
		packet[2 * i + 1] = 0xff;
	}
	sha1_hash(packet, VDUMMY_PACKET_SIZE, predef_packets_hashes[4]);

	/* Packet 5: 0x00 0x11 0x22 .. 0xff 0x00 0x11 ... */
	packet = predef_packets[5];
	for (i = 0 ; i < VDUMMY_PACKET_SIZE ; i++)
		packet[i] = ((i % 0x10) << 4) | (i % 0x10);
	sha1_hash(packet, VDUMMY_PACKET_SIZE, predef_packets_hashes[5]);

	/* Empty packets (TBD) */
	sha1_hash(predef_packets[6], VDUMMY_PACKET_SIZE, predef_packets_hashes[6]);
	sha1_hash(predef_packets[7], VDUMMY_PACKET_SIZE, predef_packets_hashes[7]);
	sha1_hash(predef_packets[8], VDUMMY_PACKET_SIZE, predef_packets_hashes[8]);
	sha1_hash(predef_packets[9], VDUMMY_PACKET_SIZE, predef_packets_hashes[9]);
	sha1_hash(predef_packets[10], VDUMMY_PACKET_SIZE, predef_packets_hashes[10]);
	sha1_hash(predef_packets[11], VDUMMY_PACKET_SIZE, predef_packets_hashes[11]);
	sha1_hash(predef_packets[12], VDUMMY_PACKET_SIZE, predef_packets_hashes[12]);
	sha1_hash(predef_packets[13], VDUMMY_PACKET_SIZE, predef_packets_hashes[13]);
	sha1_hash(predef_packets[14], VDUMMY_PACKET_SIZE, predef_packets_hashes[14]);
	sha1_hash(predef_packets[15], VDUMMY_PACKET_SIZE, predef_packets_hashes[15]);

#ifdef DEBUG
	lprintk("packgen: pre-defined packets\n");
	for (i = 0 ; i < 6 ; i++) {
		lprintk("%d:\n", i);
		packet = predef_packets[i];
		for (j = 0 ; j < VDUMMY_PACKET_SIZE ; j++)
			lprintk("%02x ", packet[j]);
		lprintk("\nhash: ");
		for (j = 0 ; j < SOOTEST_SHA1_HASH_LEN ; j++)
			lprintk("%02x ", predef_packets_hashes[i][j]);
		lprintk("\n");
	}
#endif
}

/**
 * Send a packet.
 */
void send_packet(void *buffer, size_t size) {
#ifdef DEBUG
	int i;
#endif

	//soolink_send_packet(dest_addr, buffer, VDUMMY_PACKET_SIZE);

	vdummy_generate_request(buffer);

	sha1_hash(buffer, VDUMMY_PACKET_SIZE, sent_packet_hash);

	// manual loopback
	//sootest_dummy_process_received_echo(buffer);

#ifdef DEBUG
	lprintk("hash: ");
	lprintk_buffer(sent_packet_hash, SOOTEST_SHA1_HASH_LEN);
#endif
}

/**
 *
 */
void sootest_dummy_process_received_echo(char *packet)
{
	static int packet_count = 0;
	static int iter_count = 0;
	static char test_end = 0;

#ifdef DEBUG
	int i;
#endif

	memcpy(echo_packet, packet, VDUMMY_PACKET_SIZE);

#ifdef DEBUG
	lprintk("ECHO\n");
	lprintk_buffer(echo_packet, VDUMMY_PACKET_SIZE);
#endif

	memset(echo_packet_hash, 0, SOOTEST_SHA1_HASH_LEN);

	sha1_hash(echo_packet, VDUMMY_PACKET_SIZE, echo_packet_hash);

#ifdef DEBUG
	lprintk("hash: ");
	lprintk_buffer(echo_packet_hash, SOOTEST_SHA1_HASH_LEN);
#endif

	if (!memcmp(echo_packet_hash, sent_packet_hash, SOOTEST_SHA1_HASH_LEN)) {
#ifdef DEBUG
			lprintk("OK\n");
#endif

			packet_bitmap[packet_count / 8] |= 1 << (packet_count % 8);
	}
	else {
#ifdef DEBUG
			lprintk("Error\n");
#endif
	}

	if (unlikely(iter_count >= SOOTEST_N_ITER - 1)) {
		if (unlikely(!test_end)) {
			lprintk("SOOTEST: dummy OK\n");
			test_end = 1;
		}
	}
	else {
		if (unlikely(packet_count == SOOTEST_N_PREDEF_PACKETS - 1)) {
			lprintk("SOOTEST: ");
			lprintk_buffer_separator(packet_bitmap, sizeof(packet_bitmap), '-');
			memset(packet_bitmap, 0, sizeof(packet_bitmap));
			packet_count = 0;
			iter_count++;
		}
		else
			packet_count++;
	}
}

/**
 * Compute the SHA1 hash.
 */
static void sha1_hash(void *buffer, size_t size, void *hash) {
	struct scatterlist hash_sg;
	struct crypto_hash *hash_tfm;
	struct hash_desc hash_desc;

	memset(hash, 0, SOOTEST_SHA1_HASH_LEN);

	hash_tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
	if (!hash_tfm)
		return;
	hash_desc.tfm = hash_tfm;
	hash_desc.flags = 0;
	crypto_hash_init(&hash_desc);
	sg_init_one(&hash_sg, buffer, size);
	crypto_hash_digest(&hash_desc, &hash_sg, size, hash);
	crypto_free_hash(hash_tfm);
}

void vdummy_cycle_fct(void) {
	static int i = 0;

	DBG("%s %d\n", __func__, i);
	send_packet(predef_packets[i], VDUMMY_PACKET_SIZE);
	i = (i + 1) % SOOTEST_N_PREDEF_PACKETS;
}

/**
 * Cyclic pre-defined packet sender.
 */
static int cycle_thread_fct(void *param) {
	while (!kthread_should_stop()) {
		vdummy_cycle_fct();
		msleep(SOOTEST_CYCLE_DELAY);
	}

	return 0;
}

void sootest_dummy_init(void) {
	prepare_predef_packets();
	memset(packet_bitmap, 0, sizeof(packet_bitmap));
}

void sootest_dummy_start_cycle(void) {
	cycle_thread = kthread_run(&cycle_thread_fct, NULL, "cycle_thread_fct");
}
#endif
