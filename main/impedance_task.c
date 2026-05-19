
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "unistd.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "AD5933.h"
#include "impedance_task.h"

double gain_factor;
//double system_phase;
double gain_factor_range[CALIBRATION_NUM_INCR];
double system_phase_range[CALIBRATION_NUM_INCR];
// int16_t real[NUM_INCR];
// int16_t imag[NUM_INCR];

// static led_strip_handle_t led_strip;
static int16_t real_arr[NUM_INCR];
static int16_t imag_arr[NUM_INCR];


// led_strip_config_t strip_config = {
//     .strip_gpio_num = 8, // GPIO pin for the LED strip
//     .max_leds = 1, // We only have one LED
// };

// led_strip_rmt_config_t rmt_config = {
//     .resolution_hz = 10000000, // 10 MHz resolution
// };

i2c_master_bus_config_t i2c_master_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = I2C_MASTER_SCL_IO,
    .sda_io_num = I2C_MASTER_SDA_IO,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};

void HandleSerialInput();

void print_uarr(uint8_t* arr, uint8_t n) {
    for (int i = 0; i < n; i++) {
        //ESP_LOGI("arr contents: ", "[%d] = %x", i, arr[i]);
        printf("arr contents: [%d] = %x\n", i, arr[i]);
    }
}

void print_arr(signed short* arr, uint8_t n) {
    for (int i = 0; i < n; i++) {
        //ESP_LOGI("arr contents: ", "[%d] = %d", i, arr[i]);
        printf("arr contents: [%d] = %d\n", i, arr[i]);
    }
}

void init_freq_arr(uint8_t n, uint16_t start, uint16_t step, uint16_t* arr) {
    for (int i = 0; i < n; i++) {
        arr[i] = start + step * i;
    }
}

void print_double_arr(double* arr, int n) {
    for (int i = 0; i < n; i++) {
        ESP_LOGI("arr contents: ", "[%d] = %f", i, arr[i]);
        //printf("arr contents: [%d] = %x\n", i, arr[i]);
    }
}

void init_impedance()
{
    printf("starting up\n");
    // init master bus
    // i2c_master_bus_handle_t bus_handle;

    // ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_config, &bus_handle));

    // ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    // init AD5933
    // AD5933_init_i2c_device(bus_handle);

    // reset
    AD5933_set_reg_value(AD5933_REG_CONTROL_LB, 0x18);
    
    // init calibration settings
    AD5933_init_settings(START_FREQ, INTERNAL_CLOCK_FREQ, FREQ_INCR, CALIBRATION_NUM_INCR, AD5933_RANGE_2000mVpp, AD5933_PGA_1, 25);
    
    int16_t calib_real[CALIBRATION_NUM_INCR];
    int16_t calib_imag[CALIBRATION_NUM_INCR];

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    // start frequency sweep
    AD5933_start_freq_sweep(calib_real, calib_imag);
    // calculate gain_factor based on first point recorded
    gain_factor = gain_factor_calibration(2200, calc_magnitude(calib_real[0], calib_imag[0]));
    // print the gain_factor
    ESP_LOGI("gain factor", "gainfactor: %f", gain_factor);

    // system phase calibration
    system_phase_calibration(system_phase_range, calib_real, calib_imag, CALIBRATION_NUM_INCR);
    ESP_LOGI("system phases", "listed below");
    print_double_arr(system_phase_range, CALIBRATION_NUM_INCR);
    // done with calibration

    // loop forever collecting values and logging them
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    AD5933_init_settings(START_FREQ, INTERNAL_CLOCK_FREQ, FREQ_INCR, NUM_INCR, AD5933_RANGE_2000mVpp, AD5933_PGA_1, 25);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // static int16_t real_arr[NUM_INCR];
    // static int16_t imag_arr[NUM_INCR];

}

void impedance_task() {
    while (1) {
        ESP_LOGI("log", "Starting new sweep");
        AD5933_start_freq_sweep(real_arr, imag_arr);
        for (int i = 0; i < NUM_INCR; i ++){
            ESP_LOGI("log", "Uncompensated real: %d, imag: %d", real_arr[i], imag_arr[i]);
            double impedance = AD5933_calculate_impedance(gain_factor, real_arr[i], imag_arr[i]);
            ESP_LOGI("log", "impedance magnitude: %f", impedance);
            // phase calculations
            bool phase_error;
            double phase = arctan_phase_angle(real_arr[i], imag_arr[i], &phase_error);
            double comp_real, comp_imag;
            compensated_real_and_imag(impedance, phase, system_phase_range[i], &comp_real, &comp_imag);
            
            ESP_LOGI("log", "Calculated phase: %f", phase);
            ESP_LOGI("log", "System phase compensated real: %f, imag: %f", comp_real, comp_imag);
        }
        ESP_LOGI("log",  "Pausing");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}