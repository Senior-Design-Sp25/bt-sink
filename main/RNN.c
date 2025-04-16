#include <stdio.h>
#include "rnnoise.h"
#include "esp_timer.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include <stdbool.h>

#include "common.h"
#include "RNN.h"

#define FRAME_SIZE 480
#define BUFFER_SIZE 960
#define STACK_SIZE 45000
#define RNNOISE_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define PINNED_CORE 1

// Define button GPIO pin
#define BUTTON_PIN GPIO_NUM_32

// Global boolean that will be toggled by the ISR
volatile bool denoise = true;

// Debounce variables
#define DEBOUNCE_TIME_MS 150
static volatile uint32_t last_button_press_time = 0;

TaskHandle_t rnn_handle = NULL; /* I2S audio output to DMA task handle */
RingbufHandle_t rnn_buf = NULL; /* Ringbuffer handle for audio stream */

// for tracing stack high water mark
// static UBaseType_t uxHighWaterMark;
static StaticTask_t rnnoise_task_buffer;
static StackType_t rnnoise_task_stack[ STACK_SIZE ];

typedef struct {
    DenoiseState *st;
    float *out;
    float *in;
    TaskHandle_t parent_task;
    SemaphoreHandle_t semaphore;  // Semaphore for synchronization
    bool processing_done;         // Flag to indicate processing status
} RNNoiseParams;

// Global parameters structure for the static task
static RNNoiseParams g_params;
static TaskHandle_t g_rnnoise_task_handle = NULL;

#pragma region Button

// ISR handler function
static void IRAM_ATTR button_isr_handler(void* arg)
{
    uint32_t current_time = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    
    // Simple debouncing
    if ((current_time - last_button_press_time) > DEBOUNCE_TIME_MS) {
        // Toggle the global boolean
        denoise = !denoise;
        last_button_press_time = current_time;
    }
}

void init_button(void) {
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
    //gpio_install_isr_service(0);
    
    // Add the ISR handler for the button
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);
}

#pragma endregion Button

#pragma region RNN

// Task to handle frame processing
static void process_frame_task(void *pvParameters) {
    RNNoiseParams *params = (RNNoiseParams*)pvParameters;
    
    while(1) {
        // Wait for a new frame to process
        if (xSemaphoreTake(params->semaphore, portMAX_DELAY) == pdTRUE) {
            // Process frame
            rnnoise_process_frame(params->st, params->out, params->in);
            
            // Notify parent task that processing is complete
            xTaskNotifyGive(params->parent_task);
            
            params->processing_done = true;
        }
    }
}

// Initialize the static RNNoise task
void rnnoise_init_static_task(void) {
    // Create static semaphore
    static StaticSemaphore_t semaphore_buffer;
    g_params.semaphore = xSemaphoreCreateBinaryStatic(&semaphore_buffer);
    g_params.processing_done = true;  // Start with processing_done = true
    
    // Create the static task pinned to core 1
    g_rnnoise_task_handle = xTaskCreateStaticPinnedToCore(
      process_frame_task,
      "rnnoise_task",
      STACK_SIZE,
      (void *)&g_params,
      RNNOISE_TASK_PRIORITY,
      rnnoise_task_stack,
      &rnnoise_task_buffer,
      PINNED_CORE // pin to core 1
    );
    
    if (g_rnnoise_task_handle == NULL) {
        printf("Failed to create static RNNoise task!\n");
    }
}


void rnnoise_process_frame_task_wrapper(DenoiseState *st, float *out, float *in) {
    // Wait until previous processing is done
    while (!g_params.processing_done) {
        vTaskDelay(1);  // Short delay to prevent busy waiting
    }
    
    // Clear any pending notifications before we start
    ulTaskNotifyTake(pdTRUE, 0);
    
    // Set processing parameters
    g_params.processing_done = false;
    g_params.st = st;
    g_params.out = out;
    g_params.in = in;
    g_params.parent_task = xTaskGetCurrentTaskHandle();
    
    // Signal the task to start processing
    xSemaphoreGive(g_params.semaphore);
    
    // Wait for task completion or timeout
    // This will block until our static task calls xTaskNotifyGive() for this task
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000)) == 0) {
        printf("Frame processing timeout!\n");
        g_params.processing_done = true;  // Reset the flag on timeout
    }
}

void init_rnn(void) {
    /* Setup ring buffer for audio data */
    rnn_buf = xRingbufferCreate(5000, RINGBUF_TYPE_BYTEBUF);
    if (rnn_buf == NULL) {
        ESP_LOGE(BT_SINK_TAG, "Failed to create ring buffer for RNN");
    }

    rnnoise_init_static_task();

    /* Create I2S task handler */
    BaseType_t task_ret = xTaskCreatePinnedToCore(rnn_data_handler, "RNN Task", 6000,
                    NULL, configMAX_PRIORITIES - 4, &rnn_handle, PINNED_CORE);
    if (task_ret == pdPASS) {
    ESP_LOGI(BT_SINK_TAG, "Successfully created RNN handler task");
    } else {
    ESP_LOGE(BT_SINK_TAG, "Failed to create RNN handler task: %d", task_ret);
    }
    init_button();
}

void rnn_data_handler(void *arg) {
  uint8_t buffer[BUFFER_SIZE];
  float x[FRAME_SIZE];
  size_t current_buffer_size = 0; 
  uint8_t *data = NULL;
  size_t size = 0;
  int i;

  DenoiseState* st = rnnoise_create(NULL);

  // run once without timing to init
  memset(x, 0x00, sizeof(x));
  rnnoise_process_frame_task_wrapper(st, x, x);

  for (;;) {
    size_t bytes_to_receive = BUFFER_SIZE - current_buffer_size;

    data = (uint8_t *)xRingbufferReceiveUpTo(rnn_buf, &size, (TickType_t)portMAX_DELAY, bytes_to_receive);
    
    if (size > 0) {
        // Copy received data to buffer
        memcpy(buffer + current_buffer_size, data, size);
        current_buffer_size += size;
        
        // Return the ring buffer item
        vRingbufferReturnItem(rnn_buf, (void *)data);
        
        // If buffer is full, send and reset
        if (current_buffer_size == BUFFER_SIZE) {

            int16_t *tmp = (int16_t *)buffer;

            if (denoise) {
                for (i=0;i<FRAME_SIZE;i++) x[i] = tmp[i];
                rnnoise_process_frame_task_wrapper(st, x, x);
                for (i=0;i<FRAME_SIZE;i++) tmp[i] = x[i];
            }

            xRingbufferSend(i2s_buf, buffer, BUFFER_SIZE, (TickType_t)portMAX_DELAY);
            current_buffer_size = 0;

        }
    }
  }
}

#pragma endregion RNN