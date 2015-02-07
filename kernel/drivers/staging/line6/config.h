

#ifndef CONFIG_H
#define CONFIG_H


#ifdef CONFIG_USB_DEBUG
#define DEBUG 1
#endif


#define DO_DEBUG_MESSAGES    0
#define DO_DUMP_URB_SEND     DO_DEBUG_MESSAGES
#define DO_DUMP_URB_RECEIVE  DO_DEBUG_MESSAGES
#define DO_DUMP_PCM_SEND     0
#define DO_DUMP_PCM_RECEIVE  0
#define DO_DUMP_MIDI_SEND    DO_DEBUG_MESSAGES
#define DO_DUMP_MIDI_RECEIVE DO_DEBUG_MESSAGES
#define DO_DUMP_ANY          (DO_DUMP_URB_SEND || DO_DUMP_URB_RECEIVE || \
			      DO_DUMP_PCM_SEND || DO_DUMP_PCM_RECEIVE || \
			      DO_DUMP_MIDI_SEND || DO_DUMP_MIDI_RECEIVE)
#define CREATE_RAW_FILE      0

#if DO_DEBUG_MESSAGES
#define CHECKPOINT printk(KERN_INFO "line6usb: %s (%s:%d)\n", \
			  __func__, __FILE__, __LINE__)
#endif

#if DO_DEBUG_MESSAGES
#define DEBUG_MESSAGES(x) (x)
#else
#define DEBUG_MESSAGES(x)
#endif


#endif
