

#ifndef _OSST_OPTIONS_H
#define _OSST_OPTIONS_H

#define OSST_MAX_TAPES 4

#define OSST_IN_FILE_POS 1

/* The tape driver buffer size in kilobytes. */
/* Don't change, as this is the HW blocksize */
#define OSST_BUFFER_BLOCKS 32

#define OSST_WRITE_THRESHOLD_BLOCKS 32

#define OSST_EOM_RESERVE 300

#define OSST_MAX_BUFFERS OSST_MAX_TAPES 

/* Maximum number of scatter/gather segments */
/* Fit one buffer in pages and add one for the AUX header */
#define OSST_MAX_SG      (((OSST_BUFFER_BLOCKS*1024) / PAGE_SIZE) + 1)

#define OSST_FIRST_SG    ((OSST_BUFFER_BLOCKS*1024) / PAGE_SIZE)

#define OSST_FIRST_ORDER  (15-PAGE_SHIFT)



#define OSST_TWO_FM 0

#define OSST_BUFFER_WRITES 1

#define OSST_ASYNC_WRITES 1

#define OSST_READ_AHEAD 1

#define OSST_AUTO_LOCK 0

#define OSST_FAST_MTEOM 0

#define OSST_SCSI2LOGICAL 0

#define OSST_SYSV 0


#endif
