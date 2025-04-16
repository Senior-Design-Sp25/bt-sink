#include <stdio.h>
#include "esp_bt.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt_main.h"
#include "esp_a2dp_api.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

#include "common.h"
#include "bt_sink.h"
#include "helper_functions.h"


#pragma region init_functions

void init_bt(void) {

    /* initialize NVS — it is used to store PHY calibration data */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* We only use bt classic, free bt low energy memory */
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    /* init and enable bt classic default config */
    esp_bt_controller_config_t bt_config = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_config));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    /* init and enable bluedroid stack */
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    esp_bt_cod_t cod = {
      .major = ESP_BT_COD_MAJOR_DEV_AV,
      .service = ESP_BT_COD_SRVC_AUDIO,
    };
    esp_bt_gap_set_cod(cod, ESP_BT_INIT_COD);

    ESP_LOGI(BT_SINK_TAG, "Successfully initialized bluetooth stack");

    /* Log esp BDA */
    char bda_str[18] = {0};
    ESP_LOGI(BT_SINK_TAG, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));
}

void init_gap(void) {
    /* Set device name*/
    ESP_ERROR_CHECK(esp_bt_gap_set_device_name(BT_SINK_NAME));

    /* Register gap handler */
    ESP_ERROR_CHECK(esp_bt_gap_register_callback(bt_gap_handler));

    /* Set discoverability and wait for connection*/
    ESP_ERROR_CHECK(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));
}

#pragma endregion init_functions

#pragma region handler

/* Event handler function */
void bt_gap_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *params)
{
  switch (event) {
  case ESP_BT_GAP_CONFIG_EIR_DATA_EVT:
    ESP_LOGI(BT_SINK_TAG, "gap_handler event: ESP_BT_GAP_CONFIG_EIR_DATA_EVT");
    esp_bt_status_t stat = params->config_eir_data.stat;
    if (stat == ESP_BT_STATUS_SUCCESS) {
      ESP_LOGI(BT_SINK_TAG, "params->config_eir_data.stat: ESP_BT_STATUS_SUCCESS");
    } else if (stat == ESP_BT_STATUS_EIR_TOO_LARGE) {
      ESP_LOGI(BT_SINK_TAG, "params->config_eir_data.stat: ESP_BT_STATUS_EIR_TOO_LARGE");
    } else {
      ESP_LOGI(BT_SINK_TAG, "params->config_eir_data.stat: %d", stat);
    }
    uint8_t eir_type_num = params->config_eir_data.eir_type_num;
    ESP_LOGD(BT_SINK_TAG, "params->config_eir_data.eir_type_num: %d", eir_type_num);
    for (uint8_t i = 0; i < eir_type_num; i++) {
      ESP_LOGD(BT_SINK_TAG, "params->config_eir_data.eir_type[%d]: %d",
	       i, params->config_eir_data.eir_type[i]);
    }
    break;
  case ESP_BT_GAP_MODE_CHG_EVT:
    ESP_LOGI(BT_SINK_TAG, "gap_handler event: ESP_BT_GAP_MODE_CHG_EVT");
    /* Log connected device BDA */
    char bda_str[18] = {0};
    ESP_LOGI(BT_SINK_TAG, "Own address:[%s]", bda2str((uint8_t *)params->mode_chg.bda, bda_str, sizeof(bda_str)));    
    ESP_LOGI(BT_SINK_TAG, "PM mode: %d", params->mode_chg.mode);
    break;
  default:
      ESP_LOGI(BT_SINK_TAG, "gap_handler event: %d", event);
      break;
  }
}

#pragma endregion handler