

#ifndef __VIA_MODESETTING_H__
#define __VIA_MODESETTING_H__

#include <linux/types.h>

void via_set_primary_address(u32 addr);
void via_set_secondary_address(u32 addr);
void via_set_primary_pitch(u32 pitch);
void via_set_secondary_pitch(u32 pitch);
void via_set_primary_color_depth(u8 depth);
void via_set_secondary_color_depth(u8 depth);

#endif /* __VIA_MODESETTING_H__ */
