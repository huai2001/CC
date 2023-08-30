/*
 * Copyright .Qiu<huai2011@163.com>. and other libCC contributors.
 * All rights reserved.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:

 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
*/
#include "DataList.h"
#include "Acceptor.h"

static _cc_queue_iterator_t _dataList = {0};
static _cc_queue_iterator_t _idleDataList = {0};
static _cc_mutex_t *_dataLock = NULL;

bool_t _Mir2AllocDataBuffer(const byte_t *bytes, int32_t length) {
	_cc_queue_iterator_t *it;
	_Mir2DataBuffer_t *dataBuffer;

    if (bytes == NULL || length == 0) {
        return true;
    }

	_cc_mutex_lock(_dataLock);
	it = _cc_queue_iterator_pop(&_idleDataList);
	_cc_mutex_unlock(_dataLock);

	if (it == NULL) {
		dataBuffer = (_Mir2DataBuffer_t*)_cc_malloc(sizeof(_Mir2DataBuffer_t));
		if (dataBuffer == NULL) {
			return false;
		}
	} else {
		dataBuffer = _cc_upcast(it, _Mir2DataBuffer_t, lnk);
	}

	dataBuffer->data = (byte_t*)_cc_malloc(sizeof(byte_t) * (length + 3));
	if (dataBuffer->data == NULL) {
		_cc_free(dataBuffer);
		return false;
	}

	memcpy(dataBuffer->data, bytes, length);
	dataBuffer->data[length] = 0;
    dataBuffer->length = length;

	_cc_mutex_lock(_dataLock);
	_cc_queue_iterator_push(&_dataList, &dataBuffer->lnk);
	_cc_mutex_unlock(_dataLock);
	return true;
}

bool_t _Mir2DataBufferInit(void) {
	if (_dataLock) {
		return true;
	}

	_dataLock = _cc_create_mutex();   
	if (_dataLock == NULL) {
		return false;
	}
	_cc_queue_iterator_cleanup(&_dataList);
	_cc_queue_iterator_cleanup(&_idleDataList);

	return true;
}

bool_t _Mir2DataBufferQuit(void) {
    if (_dataLock == NULL)
        return true;
	
	_cc_mutex_lock(_dataLock);

	_cc_queue_iterator_for_each(v, &_idleDataList, {
        _Mir2DataBuffer_t *dataBuffer = _cc_upcast(v, _Mir2DataBuffer_t, lnk);
        _cc_free(dataBuffer);
    });

	_cc_queue_iterator_for_each(v, &_dataList, {
		_Mir2DataBuffer_t *dataBuffer = _cc_upcast(v, _Mir2DataBuffer_t, lnk);
		_cc_free(dataBuffer->data);
		_cc_free(dataBuffer);
	});
	
	_cc_queue_iterator_cleanup(&_dataList);
	_cc_queue_iterator_cleanup(&_idleDataList);

	_cc_mutex_unlock(_dataLock);

    _cc_destroy_mutex(&_dataLock);

	return true;
}

_CC_FORCE_INLINE_ void _SendDataBuffer(_Mir2Network_t *network, pvoid_t args) {
    _Mir2DataBuffer_t *dataBuffer = (_Mir2DataBuffer_t*)args;

    _Mir2WebSocketSend(network, WS_TXTDATA, dataBuffer->data, (uint16_t)dataBuffer->length);
}

int32_t threadDataRunning(_cc_thread_t* t, pvoid_t args) {
	_cc_queue_iterator_t *it;
	_Mir2DataBuffer_t *dataBuffer;
    
    while (_Mir2GetKeepActive()) {
        _cc_mutex_lock(_dataLock);
        it = _cc_queue_iterator_pop(&_dataList);
        _cc_mutex_unlock(_dataLock);

        if (it == NULL) {
            _cc_sleep(1000);
            continue;
        }

        dataBuffer = _cc_upcast(it, _Mir2DataBuffer_t, lnk);
        _Mir2Acceptor(ServiceKind_WebSocketManager, _SendDataBuffer, dataBuffer);
        
        _cc_free(dataBuffer->data);
        dataBuffer->data = NULL;

        _cc_mutex_lock(_dataLock);
        _cc_queue_iterator_push(&_idleDataList, &dataBuffer->lnk);
        _cc_mutex_unlock(_dataLock);

        _cc_sleep(200);
    }


	return 1;
}
