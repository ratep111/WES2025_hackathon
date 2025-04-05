#include <stdio.h>
#include <string.h>
#include "driver/i2s_std.h" // Updated header
#include "esp_log.h"
#include "audio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <driver/dac.h>

#define I2S_PORT_NUM      I2S_NUM_0
#define TAG               "I2S_AUDIO"
#define SAMPLE_RATE       20000
#define DMA_BUFFER_COUNT  8
#define DMA_BUFFER_LENGTH 1024

static i2s_chan_handle_t tx_chan; // I2S channel handle

void i2s_audio_init() {
    // 1. Channel configuration
    dac_i2s_enable();
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT_NUM, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    // 2. Standard mode configuration for internal DAC
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_8BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = GPIO_NUM_0,  // Not used by internal DAC but required
            .bclk = GPIO_NUM_26, // Internal DAC uses GPIO25/26
            .ws = GPIO_NUM_25,
            .dout = GPIO_NUM_26, // Must match bclk for internal DAC
            .din = GPIO_NUM_27,  // Not used for TX
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    // 3. Initialize channel
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));

    // 4. Enable DAC mode
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
}

void audio_task(void *arg) {

    i2s_audio_init();
    const uint8_t *audio_data = mixkit_police_siren_us_1643;
    size_t audio_len          = sizeof(mixkit_police_siren_us_1643);
    size_t bytes_written      = 0;
    size_t current_pos        = 0;


    while(1) {
        ESP_LOGI(TAG, "Starting audio playback (buffer size: %d bytes)", audio_len);
        size_t remaining = audio_len - current_pos;
        size_t to_write  = (remaining > DMA_BUFFER_LENGTH) ? DMA_BUFFER_LENGTH : remaining;

        // Write audio data to I2S
        esp_err_t err = i2s_channel_write(tx_chan, audio_data + current_pos, to_write, &bytes_written, portMAX_DELAY);

        if(err != ESP_OK) {
            ESP_LOGE(TAG, "I2S write error: %s", esp_err_to_name(err));
        }

        current_pos += bytes_written;

        // Loop back to start when we reach the end
        if(current_pos >= audio_len) {
            current_pos = 0;
            ESP_LOGD(TAG, "Audio looped back to start");
        }
    }
}
