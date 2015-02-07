

#define QCIF_W  (176)
#define QCIF_H  (144)

#define CIF_W   (352)
#define CIF_H   (288)

#define LCD_X_RES	208
#define LCD_Y_RES	320
#define LCD_X_PAD	256
#define LCD_BBP		4	/* Bytes Per Pixel */

#define DISP_MAX_X_SIZE     (320)
#define DISP_MAX_Y_SIZE     (208)

#define RETURNVAL_BASE (0x400)

enum fb_ioctl_returntype {
	ENORESOURCESLEFT = RETURNVAL_BASE,
	ERESOURCESNOTFREED,
	EPROCNOTOWNER,
	EFBNOTOWNER,
	ECOPYFAILED,
	EIOREMAPFAILED,
};
