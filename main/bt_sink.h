
#ifndef __BT_SINK_H__
#define __BT_SINK_H__

#include "esp_gap_bt_api.h"

// init bt configuration
void init_bt(void);

// init bt gap handler
void init_gap(void);

// bt gap handler
void bt_gap_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *params);

#endif /* __BT_SINK_H__ */