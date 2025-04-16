
#ifndef __A2DP_SINK_H__
#define __A2DP_SINK_H__

#include "esp_a2dp_api.h"

/* init a2dp and register callback */
void init_a2dp(void);

/* a2dp event callback function */
void a2dp_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *params);

/* a2dp data callback function */
void a2dp_data_cb(const uint8_t *buf, uint32_t len);

//void init_uart(void);

//void start_audio_capture(void);

#endif /* __A2DP_SINK_H__ */