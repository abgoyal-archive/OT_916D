

#ifndef MDDI_TOSHIBA_H
#define MDDI_TOSHIBA_H

#define TOSHIBA_VGA_PRIM 1
#define TOSHIBA_VGA_SECD 2

#define LCD_TOSHIBA_2P4_VGA 	0
#define LCD_TOSHIBA_2P4_WVGA 	1
#define LCD_TOSHIBA_2P4_WVGA_PT	2
#define LCD_SHARP_2P4_VGA 	3

#define GPIO_BLOCK_BASE        0x150000
#define SYSTEM_BLOCK2_BASE     0x170000

#define GPIODIR     (GPIO_BLOCK_BASE|0x04)
#define GPIOSEL     (SYSTEM_BLOCK2_BASE|0x00)
#define GPIOPC      (GPIO_BLOCK_BASE|0x28)
#define GPIODATA    (GPIO_BLOCK_BASE|0x00)

#define write_client_reg(__X, __Y, __Z) {\
  mddi_queue_register_write(__X, __Y, TRUE, 0);\
}

#endif /* MDDI_TOSHIBA_H */
