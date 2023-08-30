#ifndef _C_DATA_LIST_H_INCLUDED_
#define _C_DATA_LIST_H_INCLUDED_

#include "Network.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Mir2DataBuffer {
	int32_t length;
	byte_t *data;
	_cc_queue_iterator_t lnk;
} _Mir2DataBuffer_t;

bool_t _Mir2DataBufferInit(void);
bool_t _Mir2DataBufferQuit(void);
bool_t _Mir2AllocDataBuffer(const byte_t *bytes, int32_t length);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif


#endif /* _C_DATA_LIST_H_INCLUDED_ */