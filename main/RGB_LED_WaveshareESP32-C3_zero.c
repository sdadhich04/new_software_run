
// other includes for ESP32 IDF etc.

#include "led_strip.h"
#include "esp_system.h"  // not sure if this one needed for LEDs??

static led_strip_handle_t led_strip;

static void setLedFromArg(uint8_t on_off)
{
    /* If the addressable LED is enabled */
    if (on_off) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        // arguments:  led_strip_set_pixel(strip handle, position in chain, R, G, B);
        //  Waveshare board has only position 0.
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);  // white, not too bright.
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}


/*
 *   Initialize the LED strip on Waveshare ESP32-C6-zero.
 * */

static void configure_led(void)
{
    ESP_LOGI(TAG, "Configure pins to blink LED_STRIP LED");

// RMT is the required config for WaveShare ESP32-C6 zero
    ESP_LOGI(TAG, "Configure LED_STRIP back-end RMT");

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

