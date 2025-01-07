/*
 * Copyright (C) 2016,2017 Daniel Rossier <daniel.rossier@soo.tech>
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

#ifndef KEYHANDLER_H
#define KEYHANDLER_H

typedef void keyhandler_fn_t(unsigned char key);

struct keyhandler {

    keyhandler_fn_t *fn;

    /* The string is not copied by register_keyhandler(), so must persist. */
    char *desc;
};

/* Initialize keytable with default handlers */
extern void initialize_keytable(void);

/*
 * Register a callback handler for key @key. The keyhandler structure is not
 * copied, so must persist.
 */
extern void register_keyhandler(unsigned char key, struct keyhandler *handler);

/* Inject a keypress into the key-handling subsystem. */
extern void handle_keypress(unsigned char key);

/* Scratch space is available for use of any keyhandler. */
extern char keyhandler_scratch[1024];

#endif /* KEYHANDLER_H */
