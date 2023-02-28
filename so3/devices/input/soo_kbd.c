/*
 * Copyright (C) 2020 Nikolaos Garanis <nikolaos.garanis@heig-vd.ch>
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

/*
 * Virtual driver for the keyboard.
 */

#if 0
#define DEBUG
#endif

#include <vfs.h>
#include <common.h>

#include <asm/io.h>

#include <device/driver.h>
#include <device/input/ps2.h>
#include <device/input/soo_kbd.h>

#include <uapi/linux/input-event-codes.h>

/* Value of the last pressed key. */
struct ps2_key last_key = {
	.value = 0,
	.state = 0
};

/* ioctl commands. */

#define GET_KEY 0
int ioctl_keyboard(int fd, unsigned long cmd, unsigned long args);

/* Device info. */

struct file_operations vkbd_fops = {
	.ioctl = ioctl_keyboard
};

struct devclass vkbd_cdev = {
	.class = DEV_CLASS_KEYBOARD,
	.type = VFS_TYPE_DEV_INPUT,
	.fops = &vkbd_fops,
};

/* Linux input event to ASCII converter. */
static uint8_t eta[] = {
/*         0     1     2     3     4     5     6     7     8     9 */
/* 0 */ 0x00, 0x1b,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',
/* 1 */  '9',  '0', '\'',  '^', 0x08, 0x09,  'q',  'w',  'e',  'r',
/* 2 */  't',  'z',  'u',  'i',  'o',  'p', 0x00, 0x00, 0x0a, 0x00,
/* 3 */  'a',  's',  'd',  'f',  'g',  'h',  'j',  'k',  'l', 0x00,
/* 4 */ 0x00, 0x00, 0x00,  '$',  'y',  'x',  'c',  'v',  'b',  'n',
/* 5 */  'm',  ',',  '.',  '-', 0x00, 0x00, 0x00, 0x20, 0x00, 0x00,
/* 6 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 7 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  '<', 0x00, 0x00, 0x00,
/* 9 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0 */ 0x00, 0x00, 0x00, 0x11, 0x00, 0x14, 0x13, 0x00, 0x12, 0x00,
/* 1 */ 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static uint8_t s_eta[] = {
/*         0     1     2     3     4     5     6     7     8     9 */
/* 0 */ 0x00, 0x00,  '+',  '"',  '*', 0x00,  '%',  '&',  '/',  '(',
/* 1 */  ')',  '=',  '?',  '`', 0x00, 0x00,  'Q',  'W',  'E',  'R',
/* 2 */  'T',  'Z',  'U',  'I',  'O',  'P', 0x00, 0x00, 0x00, 0x00,
/* 3 */  'A',  'S',  'D',  'F',  'G',  'H',  'J',  'K',  'L', 0x00,
/* 4 */ 0x00, 0x00, 0x00, 0x00,  'Y',  'X',  'C',  'V',  'B',  'N',
/* 5 */  'M',  ';',  ':',  '_', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 6 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 7 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  '>', 0x00, 0x00, 0x00,
};

void soo_input_event(unsigned int type, unsigned int code, int value)
{
	/* Ignore events which do not come from a key. */
	if (type != EV_KEY) {
		return;
	}

	/* Handling of left/right shift key. */
	if (code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT) {
		last_key.value = 0;
		last_key.state ^= KEY_ST_SHIFT;
		return;
	}

	/*
	 * Ignore "key released" events. A key is released in the ioctl so the
	 * client can read it.
	 */
	if (!value) {
		return;
	}

	if (last_key.state & KEY_ST_SHIFT) {
		last_key.value = s_eta[code];
	}
	else {
		last_key.value = eta[code];
	}

	last_key.state |= KEY_ST_PRESSED;
}

int ioctl_keyboard(int fd, unsigned long cmd, unsigned long args)
{
	switch (cmd) {

	case GET_KEY:
		*((struct ps2_key *) args) = last_key;
		/* Indicate we have read the key and don't need to read it again. */
		last_key.value = 0;
		break;

	default:
		/* Unknown command. */
		return -1;
	}

	return 0;
}

int init_keyboard(dev_t *dev)
{
	/* Register the input device so it can be accessed from user space. */
	devclass_register(dev, &vkbd_cdev);
	return 0;
}

REGISTER_DRIVER_POSTCORE("keyboard,soo_input", init_keyboard);
