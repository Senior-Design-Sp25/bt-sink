#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2s_common.h"
#include "driver/i2s_types.h"
#include "driver/i2s_std.h"
#include "hal/i2s_types.h"

#include "common.h"
#include "i2s_sink.h"

/* Variables */
i2s_chan_handle_t tx_channel;	/* I2S handle for audio channel (v5.0 API) */
TaskHandle_t i2s_handle = NULL; /* I2S audio output to DMA task handle */
RingbufHandle_t i2s_buf = NULL; /* Ringbuffer handle for audio stream */

#pragma region init

void init_i2s(void) {
  i2s_chan_config_t channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  channel_config.auto_clear = true; /* Automatically clear DMA TX
				       buffer */
  i2s_std_config_t standard_config = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
    .slot_cfg =  I2S_STD_PCM_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = I2S_GPIO_BCLK,
      .ws = I2S_GPIO_WS,
      .dout = I2S_GPIO_D_OUT,
      .din = I2S_GPIO_UNUSED,
      .invert_flags = {
	.mclk_inv = false,
	.bclk_inv = false,
	.ws_inv = false,
      },
    },
  };

    ESP_ERROR_CHECK(i2s_new_channel(&channel_config, &tx_channel, NULL));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_channel, &standard_config));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_channel));
    ESP_LOGI(BT_SINK_TAG, "Successfully initialized I2S tx");

    /* Setup ring buffer for audio data */
    i2s_buf = xRingbufferCreate(2000, RINGBUF_TYPE_BYTEBUF);
    if (i2s_buf == NULL) {
        ESP_LOGE(BT_SINK_TAG, "Failed to create ring buffer for I2S");
    }

    /* Create I2S task handler */
    BaseType_t task_ret = xTaskCreatePinnedToCore(i2s_data_handler, "I2S Task", 1024,
                    NULL, configMAX_PRIORITIES - 3, &i2s_handle, 1);
    if (task_ret == pdPASS) {
    ESP_LOGI(BT_SINK_TAG, "Successfully created I2S handler task");
    } else {
    ESP_LOGE(BT_SINK_TAG, "Failed to create I2S handler task: %d", task_ret);
    }
}

#pragma endregion init

#pragma region I2S

void i2s_data_handler(void *arg) {
  uint8_t *data = NULL;
  size_t size = 0;
  size_t bytes_written = 0;

  for (;;) {
    data = (uint8_t *)xRingbufferReceive(i2s_buf, &size, (TickType_t)portMAX_DELAY);
    if (size != 0) {
      i2s_channel_write(tx_channel, data, size, &bytes_written, portMAX_DELAY);
      vRingbufferReturnItem(i2s_buf, (void *)data);
    }
  }
}

#pragma endregion I2S