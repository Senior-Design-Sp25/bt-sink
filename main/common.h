
#ifndef __COMMON_H__
#define __COMMON_H__

#include "freertos/ringbuf.h"
#include "esp_heap_caps.h"

/* DEFINITIONS */
#define BT_SINK_TAG "BT_SINK"
#define BT_SINK_NAME "ESP_BT_SINK"

/* Ringbuffer handle for audio stream */
extern RingbufHandle_t i2s_buf;
extern RingbufHandle_t rnn_buf;

void print_initial_memory();

void print_memory_usage();

void print_max_ram();


#endif /* __COMMON_H__ */