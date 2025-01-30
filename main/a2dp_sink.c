#include "esp_err.h"
#include "esp_log.h"
#include "esp_a2dp_api.h"

#include "common.h"
#include "a2dp_sink.h"
#include "helper_functions.h"

/* init a2dp and register callback */
void init_a2dp(void) {
    ESP_ERROR_CHECK(esp_a2d_sink_init());
    ESP_ERROR_CHECK(esp_a2d_register_callback(a2dp_cb));
    ESP_ERROR_CHECK(esp_a2d_sink_register_data_callback(a2dp_data_cb));
    ESP_LOGI(BT_SINK_TAG, "Successfully initialized A2DP sink");
}

#pragma region cb

/* a2dp event callback function */
void a2dp_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *params) {
    char bda_str[18] = {0};
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
    esp_a2d_connection_state_t state = params->conn_stat.state;
    if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
      ESP_LOGI(BT_SINK_TAG, "Connection state: ESP_A2D_CONNECTION_STATE_DISCONNECTED");
    } else if (state == ESP_A2D_CONNECTION_STATE_CONNECTING) {
      ESP_LOGI(BT_SINK_TAG, "Connection state: ESP_A2D_CONNECTION_STATE_CONNECTING");
    } else if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
      ESP_LOGI(BT_SINK_TAG, "Connection state: ESP_A2D_CONNECTION_STATE_CONNECTED");
    } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTING) {
      ESP_LOGI(BT_SINK_TAG, "Connection state: ESP_A2D_CONNECTION_STATE_DISCONNECTING");
    }
    
    /* Log Remote device BDA */
    ESP_LOGI(BT_SINK_TAG, "Remote Bluetooth Address: %s", bda2str((uint8_t *)params->conn_stat.remote_bda, bda_str, sizeof(bda_str)));
    break;

    case ESP_A2D_AUDIO_STATE_EVT:

    case ESP_A2D_AUDIO_CFG_EVT:
        ESP_LOGI(BT_SINK_TAG, "a2d_sink_handler event: ESP_A2D_AUDIO_CFG_EVT");

        /* Log Remote device BDA */
        ESP_LOGI(BT_SINK_TAG, "Remote Bluetooth Address: %s", bda2str((uint8_t *)params->conn_stat.remote_bda, bda_str, sizeof(bda_str)));

        /* Determine remote device sample rate and channel number */
        int sample_rate = 16000;
        int ch_count = 2;
        char oct0 = params->audio_cfg.mcc.cie.sbc[0];
        if (oct0 & (0x01 << 6)) {
            sample_rate = 32000;
        } else if (oct0 & (0x01 << 5)) {
            sample_rate = 44100;
        } else if (oct0 & (0x01 << 4)) {
            sample_rate = 48000;
        }

        if (oct0 & (0x01 << 3)) {
            ch_count = 1;
        }
        ESP_LOGI(BT_SINK_TAG, "Sample rate: %d", sample_rate);
        ESP_LOGI(BT_SINK_TAG, "Channel mode: %d", ch_count);
        break;

    case ESP_A2D_PROF_STATE_EVT:
    case ESP_A2D_SNK_PSC_CFG_EVT:
    case ESP_A2D_SNK_SET_DELAY_VALUE_EVT:
    case ESP_A2D_SNK_GET_DELAY_VALUE_EVT:
    default:
        ESP_LOGE(BT_SINK_TAG, "Invalid A2DP event: %d", event);
        break;
    }
}

#pragma endregion cb

/* a2dp data callback function */
void a2dp_data_cb(const uint8_t *buf, uint32_t len) {
  //ESP_LOGI(BT_SINK_TAG, "We've received %lu bytes of music data!", len);
  // Send data to ring buffer
  xRingbufferSend(i2s_buf, (void *)buf, len, (TickType_t)portMAX_DELAY);
}