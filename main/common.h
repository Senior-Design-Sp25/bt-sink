
#ifndef __COMMON_H__
#define __COMMON_H__

#include "freertos/ringbuf.h"

/* DEFINITIONS */
#define BT_SINK_TAG "BT_SINK"
#define BT_SINK_NAME "ESP_BT_SINK"

/* Ringbuffer handle for audio stream */
extern RingbufHandle_t i2s_buf;


#endif /* __COMMON_H__ */