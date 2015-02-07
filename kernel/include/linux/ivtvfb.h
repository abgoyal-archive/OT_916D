

#ifndef __LINUX_IVTVFB_H__
#define __LINUX_IVTVFB_H__

#include <linux/compiler.h>
#include <linux/types.h>

/* Framebuffer external API */

struct ivtvfb_dma_frame {
	void __user *source;
	unsigned long dest_offset;
	int count;
};

#define IVTVFB_IOC_DMA_FRAME 	_IOW('V', BASE_VIDIOC_PRIVATE+0, struct ivtvfb_dma_frame)

#endif
