
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

#include "timer_Photo.h"

#define TAG "TPT Timer/Photonics Task: "
#define isrTAG "ISR: "

/*   29-Dec-2025  BH
 *   New approach: use timer-driven ISR to get control of wave frequency
 *   and sampling times.
 */

//
// Forward declarations
//

//  ISR for timer
static bool timer_isr_callback(gptimer_handle_t timer,
                               const gptimer_alarm_event_data_t *edata,
                               void *user_ctx);

// Sync semaphore for ISR w state_machine_task
SemaphoreHandle_t acquisition_complete_sem = NULL;

// my ADC function
uint16_t read_adc(void);

// Transfer samples from ISR mem to task
static void get_latest_samples(uint16_t*, uint16_t*);

// Spinlock for protecting sample data
static portMUX_TYPE samples_mux = portMUX_INITIALIZER_UNLOCKED;

// ADC handle
adc_oneshot_unit_handle_t adc1_handle;

// Globals
uint32_t sensing_cycle_count = 0;
uint64_t next_alarm_count=1000;  // set to some value to avoid warning
uint8_t  phase = EXCITATION_OFF;

// data buffers to transfer batch of samples to the state_machine task.
volatile uint16_t  data_buffer[PHOTO_DATA_BUF_SIZE]={0xFFFF};  // where data will be stored
volatile uint16_t *data_ptr = data_buffer;   // pointer for async writing/reading buff.

volatile uint8_t phase_buffer[PHOTO_DATA_BUF_SIZE]={0xFF};  // where phase tag will be stored
volatile uint8_t *phase_ptr = phase_buffer;   // pointer for async writing/reading buff.


// Globals for ISR
gptimer_handle_t gptimer = NULL;
volatile timer_state_t isr_state = STATE_GPIO_TOGGLE;
volatile uint8_t gpio_level = 0; // must have this init value to start with ON pulse

// below are small old buffers for early tests -
volatile uint16_t samples_positive[SAMPLES_PER_PHASE];
volatile uint16_t samples_zero[SAMPLES_PER_PHASE];
volatile uint16_t samples_positive[SAMPLES_PER_PHASE];
volatile uint16_t samples_zero[SAMPLES_PER_PHASE];

esp_err_t init_photonics(void) {
    esp_err_t statusCode = 0; // 0== normal
    //
    //   1) set PIN_EXCIT_DRIVE to voltage output
    //
    //Configure the Excitation LED pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_EXCIT_DRIVE),  // Bitmask of pins
        .mode = GPIO_MODE_OUTPUT,                    // Set as output
        .pull_up_en = GPIO_PULLUP_DISABLE,          // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,      // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE              // Disable interrupts
    };
    // send the config to hardware
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Excitation Pin Configured");

    //
    //   2) set TPT_PIN_ADC_PD to input to the ADC
    //

    // Configure ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    // Configure the channel
    adc_oneshot_chan_cfg_t AD_chan_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,  // 12-bit for ESP32-C6
        .atten = TPT_ADC_ATTEN
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_2, &AD_chan_config);
    ESP_LOGI(TAG, "ADC Configured");

    // ADC Documentation ref:
    //   connect ADC  https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32/api-reference/peripherals/adc.html

    // Initialize the Timer
    //     Claide.ai helped

     // Timer configuration - NO DIVIDER in v5.x!
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION_HZ,  // 1MHz resolution
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    // Register callback
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_isr_callback
        };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));

    // Enable timer
    ESP_ERROR_CHECK(gptimer_enable(gptimer));


   // start_timer(gptimer);

    return statusCode;
    }


void start_timer(gptimer_handle_t gptimer){

    // Set first alarm to start quickly
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 2500,  // 100µs
        .flags.auto_reload_on_alarm = true,
    };

    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    // Start timer
    ESP_ERROR_CHECK(gptimer_start(gptimer));
    }

    /*
     * thanks Claude!
     */
void init_acquisition_semaphore(void) {
    if (acquisition_complete_sem == NULL) {
        acquisition_complete_sem = xSemaphoreCreateBinary();
        if (acquisition_complete_sem == NULL) {
            ESP_LOGE(TAG, "Failed to create acquisition semaphore");
        }
    }
}

/************************************************************************   ISR
 *
 *  ISR is static for fast (in-ram) execution
 *     Claide.ai helped
 */
static bool IRAM_ATTR timer_isr_callback(gptimer_handle_t timer,
                                         const gptimer_alarm_event_data_t *edata,
                                         void *user_ctx)  {
    int idx=0;
    uint16_t tmp=0;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    switch(isr_state) {
        case STATE_GPIO_TOGGLE:
            // Toggle GPIO
            gpio_level = !gpio_level;  // note 1st time through = ON
            gpio_set_level(OUTPUT_GPIO, gpio_level);
            if(gpio_level) {
                phase = EXCITATION_ON;
                }
            else {
                phase = EXCITATION_OFF;
                }

            // ESP_EARLY_LOGI(isrTAG, "got here - TOGGLE");

            // Schedule first sample in middle of phase
            next_alarm_count = edata->alarm_value + SAMPLE_DELAY_US;
            isr_state = STATE_SAMPLE_1;
            break;

        case STATE_SAMPLE_1:
            // Take first A/D sample
            idx=0;
            if (gpio_level == 1) {
                tmp = read_adc();
                *data_ptr = tmp;
                samples_positive[idx]=tmp;
            } else {
                tmp = read_adc();
                *data_ptr= tmp;
                samples_zero[idx]=tmp;
            }
            *phase_ptr = phase;
            next_alarm_count = edata->alarm_value + INTER_SAMPLE_US;
            data_ptr++;
            phase_ptr++;
            isr_state = STATE_SAMPLE_2;
            break;

        case STATE_SAMPLE_2:
            // Take second A/D sample
            idx=1;
            if (gpio_level == 1) {
                tmp = read_adc();
                *data_ptr = tmp;
                samples_positive[idx]=tmp;
            } else {
                tmp = read_adc();
                *data_ptr= tmp;
                samples_zero[idx]=tmp;
            }
            *phase_ptr = phase;

            next_alarm_count = edata->alarm_value + INTER_SAMPLE_US;
            data_ptr++;
            phase_ptr++;
            isr_state = STATE_SAMPLE_3;
            break;

        case STATE_SAMPLE_3:
            // Take third A/D sample
            idx=2;
            if (gpio_level == 1) {
                tmp = read_adc();
                *data_ptr = tmp;
                samples_positive[idx]=tmp;
            } else {
                tmp = read_adc();
                *data_ptr= tmp;
                samples_zero[idx]=tmp;
            }
            *phase_ptr = phase;


            // Calculate remaining time until next GPIO toggle
            next_alarm_count = edata->alarm_value +
                              (PHASE_DURATION_US - SAMPLE_DELAY_US - 2*INTER_SAMPLE_US - 50);
            isr_state = STATE_GPIO_TOGGLE;
            if (phase==EXCITATION_OFF){
                sensing_cycle_count++;  //  count complete cycles (on+off phases)
                }
            data_ptr++;
            phase_ptr++;
            break;
    }

    if (sensing_cycle_count< SENSING_CYCLES_NUM){
    // if (sensing_cycle_count < 20){  //  simpler testing
        // Set next alarm
        gptimer_alarm_config_t alarm_config = {
            .alarm_count = next_alarm_count,
            .flags.auto_reload_on_alarm = false,
        };
        gptimer_set_alarm_action(timer, &alarm_config);
    }
    else {
        xSemaphoreGiveFromISR(acquisition_complete_sem, &xHigherPriorityTaskWoken);

    }
    // else - timer does not cause any more interrupts.

    return xHigherPriorityTaskWoken; // rec'd by Claude over false
    // return to interrupted task (true = switch to highest prio task)
   }

void photonic_task(void*) {
    /*
    int flag = 1;
    unsigned long int on_total = 0;
    unsigned long int off_total = 0;

    int64_t timeused = 0;
    int64_t pulseStart = 0;
    int i = 0;  // Added missing semicolon */
    uint16_t pos[3], low[3];  // places to store data
    int cycleCnt = 0;

    while(1) {
        cycleCnt++;

        // Get latest samples
        get_latest_samples(pos, low);

        printf("Run Cycle %d - Positive phase: %d, %d | Zero phase: %d, %d\n",
                cycleCnt, pos[0], pos[1], low[0], low[1]);
        ESP_LOGI(TAG, "photonic task is alive: %d/%d",cycleCnt, (int)sensing_cycle_count);
        vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

// Retrieve samples from main code
void get_latest_samples(uint16_t *pos_samples, uint16_t *zero_samples)
{
    portENTER_CRITICAL(&samples_mux);
    for (int i = 0; i < SAMPLES_PER_PHASE; i++) {
        pos_samples[i] = samples_positive[i];
        zero_samples[i] = samples_zero[i];
    }
    portEXIT_CRITICAL(&samples_mux);
}


uint16_t read_adc(void){
    int n_readSum = 0;
    n_readSum = collect_PD_ADC(2);
    return n_readSum >> 1;  // average of 2 readings (~22uSec)
    }


//
//    Collect one test A/D sample
//
unsigned long int photonic_test(void) {
    return collect_PD_ADC(1);
    }

unsigned long int collect_PD_ADC(int n) {
    int adc_raw;
    int total=0;
    for (int i=0;i<n;i++) {
        // Read raw ADC value (0-4095 for 12-bit) n times.
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_2, &adc_raw);
        total += adc_raw;
        }
    return total; // return value.
    }
