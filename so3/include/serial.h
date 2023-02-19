/*
 * Copyright (C) 2016 Daniel Rossier <daniel.rossier@heig-vd.ch>
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

#ifndef SERIAL_H
#define SERIAL_H

#include <spinlock.h>

struct cpu_user_regs;

/* Register a character-receive hook on the specified COM port. */
typedef void (*serial_rx_fn)(char, struct cpu_user_regs *);
void serial_set_rx_handler(int handle, serial_rx_fn fn);

/* Number of characters we buffer for a polling receiver. */
#define serial_rxbufsz 32

/* Number of characters we buffer for an interrupt-driven transmitter. */
extern unsigned int serial_txbufsz;

struct uart_driver;

struct serial_port {
    /* Uart-driver parameters. */
    struct uart_driver *driver;
    void               *uart;
    /* Number of characters the port can hold for transmit. */
    int                 tx_fifo_size;
    /* Transmit data buffer (interrupt-driven uart). */
    char               *txbuf;
    unsigned int        txbufp, txbufc;
    bool                tx_quench;
    int                 tx_log_everything;
    /* Force synchronous transmit. */
    int                 sync;
    /* Receiver callback functions (asynchronous receivers). */
    serial_rx_fn        rx_lo, rx_hi, rx;
    /* Receive data buffer (polling receivers). */
    char                rxbuf[serial_rxbufsz];
    unsigned int        rxbufp, rxbufc;
    /* Serial I/O is concurrency-safe. */
    spinlock_t          rx_lock, tx_lock;
};

struct uart_driver {
    /* Put a character onto the serial line. */
    void (*putc)(struct serial_port *, char);
};

/* 'Serial handles' are composed from the following fields. */
#define SERHND_IDX      (1<<0) /* COM1 or COM2?                           */
#define SERHND_HI       (1<<1) /* Mux/demux each transferred char by MSB. */
#define SERHND_LO       (1<<2) /* Ditto, except that the MSB is cleared.  */
#define SERHND_COOKED   (1<<3) /* Newline/carriage-return translation?    */



/* Transmit a single character via the specified COM port. */
void serial_putc(int handle, char c);

/* Transmit a NULL-terminated string via the specified COM port. */
void serial_puts(const char *s);

/*
 * An alternative to registering a character-receive hook. This function
 * will not return until a character is available. It can safely be
 * called with interrupts disabled.
 */
char serial_getc(int handle);

/* Forcibly prevent serial lockup when the system is in a bad way. */
/* (NB. This also forces an implicit serial_start_sync()). */
void serial_force_unlock(int handle);



/* Return irq number for specified serial port (identified by index). */
int serial_irq(int idx);

/* Serial suspend/resume. */
void serial_suspend(void);
void serial_resume(void);

/*
 * Initialisation and helper functions for uart drivers.
 */
/* Register a uart on serial port @idx (e.g., @idx==0 is COM1). */
void serial_register_uart(int idx, struct uart_driver *driver, void *uart);


/*
 * Initialisers for individual uart drivers.
 */
/* NB. Any default value can be 0 if it is unknown and must be specified. */
struct ns16550_defaults {
    int baud;      /* default baud rate; BAUD_AUTO == pre-configured */
    int data_bits; /* default data bits (5, 6, 7 or 8) */
    int parity;    /* default parity (n, o, e, m or s) */
    int stop_bits; /* default stop bits (1 or 2) */
    int irq;       /* default irq */
    unsigned long io_base; /* default io_base address */
};
void ns16550_init(int index, struct ns16550_defaults *defaults);

/* Baud rate was pre-configured before invoking the UART driver. */
#define BAUD_AUTO (-1)

#endif /* SERIAL_H */
