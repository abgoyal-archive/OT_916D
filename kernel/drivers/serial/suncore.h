

#ifndef _SERIAL_SUN_H
#define _SERIAL_SUN_H

/* Serial keyboard defines for L1-A processing... */
#define SUNKBD_RESET		0xff
#define SUNKBD_L1		0x01
#define SUNKBD_UP		0x80
#define SUNKBD_A		0x4d

extern unsigned int suncore_mouse_baud_cflag_next(unsigned int, int *);
extern int suncore_mouse_baud_detection(unsigned char, int);

extern int sunserial_register_minors(struct uart_driver *, int);
extern void sunserial_unregister_minors(struct uart_driver *, int);

extern int sunserial_console_match(struct console *, struct device_node *,
				   struct uart_driver *, int, bool);
extern void sunserial_console_termios(struct console *,
				      struct device_node *);

#endif /* !(_SERIAL_SUN_H) */
