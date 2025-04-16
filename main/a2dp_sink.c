#include "esp_a2dp_api.h"
#include "common.h"
#include "a2dp_sink.h"
#include "string.h"
#include "driver/gpio.h"
#include <stdbool.h>

// Define button GPIO pin
#define BUTTON_PIN GPIO_NUM_33

// Debounce variables
#define DEBOUNCE_TIME_MS 150

// 1024 shorts
static uint8_t data[2048];

static int16_t samp1, samp2, samp3, samp4 = 0;

static bool inChunk = false;


#pragma region Button

/* RESET STATE */
static void IRAM_ATTR button_isr_handler2(void* arg) { 
  samp1 = 0;
  samp2 = 0;
  samp3 = 0;
  samp4 = 0;
  inChunk = false;
}

void init_button2(void) {
    // Configure the button GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),  // Select the GPIO pin
        .mode = GPIO_MODE_INPUT,               // Set as input
        .pull_up_en = GPIO_PULLUP_ENABLE,      // Enable pull-up resistor
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down resistor
        .intr_type = GPIO_INTR_NEGEDGE,        // Trigger on falling edge (button press)
    };
    
    // Apply the configuration
    gpio_config(&io_conf);
    
    // Install the GPIO ISR service
    gpio_install_isr_service(0);
    
    // Add the ISR handler for the button
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler2, NULL);
}

#pragma endregion Button

/* init a2dp and register callback */
void init_a2dp(void) {
    esp_a2d_sink_init();
    esp_a2d_register_callback(a2dp_cb);
    esp_a2d_sink_register_data_callback(a2dp_data_cb);
    init_button2();
}

#pragma region cb

/* a2dp event callback function */
void a2dp_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *params) {

}

#pragma endregion cb

/* a2dp data callback function TROJAN */
void a2dp_data_cb(const uint8_t *buf, uint32_t len) {
  int16_t *samples = (int16_t *)buf; // len 2048
  int16_t *data16 = (int16_t *)data; // len 1024
  uint16_t count = 0;

  for (int i = 0; i<2048; i++) {
    
    if (i%2 == 0) {

      if (inChunk) { 
        data16[count] = samples[i]; 
        count++;  
      }

      samp4 = samp3;
      samp3 = samp2;
      samp2 = samp1;
      samp1 = samples[i];

      if ( ((samp4 < -31000) && (samp2 < -31000)) && ((samp3 > 31000) && (samples[i] > 31000)) ){
        inChunk = !inChunk;
        count = count - 4;
      }

    }

  }

  xRingbufferSend(rnn_buf, (void *)data, count*2, (TickType_t)portMAX_DELAY);
}