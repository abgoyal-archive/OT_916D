

#ifndef AUDIO_H
#define AUDIO_H


#include "driver.h"


extern void line6_cleanup_audio(struct usb_line6 *);
extern int line6_init_audio(struct usb_line6 *);
extern int line6_register_audio(struct usb_line6 *);


#endif
