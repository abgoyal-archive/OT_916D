

#ifndef _GPIO_MOUSE_H
#define _GPIO_MOUSE_H

#define GPIO_MOUSE_POLARITY_ACT_HIGH	0x00
#define GPIO_MOUSE_POLARITY_ACT_LOW	0x01

#define GPIO_MOUSE_PIN_UP	0
#define GPIO_MOUSE_PIN_DOWN	1
#define GPIO_MOUSE_PIN_LEFT	2
#define GPIO_MOUSE_PIN_RIGHT	3
#define GPIO_MOUSE_PIN_BLEFT	4
#define GPIO_MOUSE_PIN_BMIDDLE	5
#define GPIO_MOUSE_PIN_BRIGHT	6
#define GPIO_MOUSE_PIN_MAX	7

struct gpio_mouse_platform_data {
	int scan_ms;
	int polarity;

	union {
		struct {
			int up;
			int down;
			int left;
			int right;

			int bleft;
			int bmiddle;
			int bright;
		};
		int pins[GPIO_MOUSE_PIN_MAX];
	};
};

#endif /* _GPIO_MOUSE_H */
