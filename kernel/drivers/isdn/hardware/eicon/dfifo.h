

#ifndef __DIVA_IDI_DFIFO_INC__
#define __DIVA_IDI_DFIFO_INC__
#define DIVA_DFIFO_CACHE_SZ   64 /* Used to isolate pipe from
                    rest of the world
                   should be divisible by 4
                   */
#define DIVA_DFIFO_RAW_SZ    (2512*8)
#define DIVA_DFIFO_DATA_SZ   68
#define DIVA_DFIFO_HDR_SZ    4
#define DIVA_DFIFO_SEGMENT_SZ  (DIVA_DFIFO_DATA_SZ+DIVA_DFIFO_HDR_SZ)
#define DIVA_DFIFO_SEGMENTS   ((DIVA_DFIFO_RAW_SZ)/(DIVA_DFIFO_SEGMENT_SZ)+1)
#define DIVA_DFIFO_MEM_SZ (\
        (DIVA_DFIFO_SEGMENT_SZ)*(DIVA_DFIFO_SEGMENTS)+\
        (DIVA_DFIFO_CACHE_SZ)*2\
             )
#define DIVA_DFIFO_STEP DIVA_DFIFO_SEGMENT_SZ
#define DIVA_DFIFO_WRAP   0x80 /* This is the last block in fifo   */
#define DIVA_DFIFO_READY  0x40 /* This block is ready for processing */
#define DIVA_DFIFO_LAST   0x20 /* This block is last in message      */
#define DIVA_DFIFO_AUTO   0x10 /* Don't look for 'ready', don't ack */
int diva_dfifo_create (void* start, int length);
#endif
