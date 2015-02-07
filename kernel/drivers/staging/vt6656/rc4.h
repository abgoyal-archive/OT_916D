

#ifndef __RC4_H__
#define __RC4_H__

#include "ttype.h"

/*---------------------  Export Definitions -------------------------*/
/*---------------------  Export Types  ------------------------------*/
typedef struct {
    unsigned int ux;
    unsigned int uy;
    BYTE abystate[256];
} RC4Ext, *PRC4Ext;

void rc4_init(PRC4Ext pRC4, PBYTE pbyKey, unsigned int cbKey_len);
unsigned int rc4_byte(PRC4Ext pRC4);
void rc4_encrypt(PRC4Ext pRC4, PBYTE pbyDest, PBYTE pbySrc,
		 unsigned int cbData_len);

#endif /* __RC4_H__ */
