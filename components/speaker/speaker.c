#include <stdio.h>
#include <string.h>
#include "driver/i2s.h"
#include "esp_log.h"
#include "audio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <driver/dac.h>

#define I2S_PORT    I2S_NUM_0
#define TAG         "I2S_AUDIO"
#define SAMPLE_RATE 20000

// Queue for buffer management
static QueueHandle_t i2s_event_queue;

void i2s_dac_init() {
    i2s_config_t i2s_config = {
        .mode                 = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
        .sample_rate          = SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_8BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 8,
        .dma_buf_len          = 1024,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
    };

    // Create a queue to handle I2S event callbacks
    i2s_event_queue = xQueueCreate(10, sizeof(i2s_event_t));

    ESP_ERROR_CHECK(i2s_driver_install(I2S_PORT, &i2s_config, 10, &i2s_event_queue));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_PORT, NULL));               // Use internal DAC
    ESP_ERROR_CHECK(i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN)); // DAC_1 (GPIO25)
}

void audio_task(void *arg) {
    // Skip the initial silent part (approximately 500 bytes)
    size_t skip_bytes         = 500;
    const uint8_t *audio_data = mixkit_police_siren_us_1643 + skip_bytes;
    size_t audio_len          = sizeof(mixkit_police_siren_us_1643) - skip_bytes;

    // Create a scaled copy of the audio data for better volume
    // uint8_t *scaled_audio = malloc(audio_len);
    // if(!scaled_audio) {
    //     ESP_LOGE(TAG, "Failed to allocate memory for scaled audio");
    //     vTaskDelete(NULL);
    //     return;
    // }

    // // Scale up the volume
    // for(size_t i = 0; i < audio_len; i++) {
    //     // Scale up by 1.5x, clipping at 255
    //     uint16_t scaled = (uint16_t) audio_data[i] * 1.5;
    //     scaled_audio[i] = (scaled > 255) ? 255 : scaled;
    // }

    ESP_LOGI(TAG, "Starting audio playback, length: %d bytes", audio_len);

    size_t bytes_written = 0;
    size_t current_pos   = 0;

    // Initial buffer fill
    i2s_write(I2S_PORT, audio_data, audio_len > 4096 ? 4096 : audio_len, &bytes_written, portMAX_DELAY);
    current_pos += bytes_written;

    while(1) {
        i2s_event_t event;
        if(xQueueReceive(i2s_event_queue, &event, portMAX_DELAY)) {
            if(event.type == I2S_EVENT_TX_DONE) {
                size_t remaining = audio_len - current_pos;
                if(remaining > 0) {
                    // Write next chunk of data
                    size_t to_write = (remaining > 4096) ? 4096 : remaining;
                    i2s_write(I2S_PORT, audio_data + current_pos, to_write, &bytes_written, 0);
                    current_pos += bytes_written;
                } else {
                    // Loop back to start
                    current_pos = 0;
                    ESP_LOGI(TAG, "Audio looped back to start");
                    i2s_write(I2S_PORT, audio_data, audio_len > 4096 ? 4096 : audio_len, &bytes_written, 0);
                    current_pos += bytes_written;
                }
            }
        }
    }

    // This code will never be reached, but good practice to include
    // free(audio_data);
    vTaskDelete(NULL);
}