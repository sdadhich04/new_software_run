#ifndef PHOTONIC_TASK_H //prevent double includes
#define PHOTONIC_TASK_H

#include "esp_log.h"
#include "unistd.h"
#include "driver/gptimer.h"
#include "esp_timer.h"
#include "freertos/semphr.h"

// LED output
#define PIN_EXCIT_DRIVE   GPIO_NUM_0   //GPIO-00, module pin 3
#define OUTPUT_GPIO       PIN_EXCIT_DRIVE

// ADC parameters:

// The options for attenuation
// ADC_ATTEN_DB_0      // 0 dB attenuation, range: 0 - ~750 mV
// ADC_ATTEN_DB_2_5    // 2.5 dB attenuation, range: 0 - ~1050 mV
// ADC_ATTEN_DB_6      // 6 dB attenuation, range: 0 - ~1300 mV
// ADC_ATTEN_DB_12     // 12 dB attenuation, range: 0 - ~3100 mV

#define TPT_PIN_ADC_PD    GPIO_NUM_2        //GPIO-2, module pin 5
#define ADC_CHANNEL       ADC_CHANNEL_2     // ADC channel for GPIO2
#define TPT_ADC_ATTEN     ADC_ATTEN_DB_12   // 0-3.1V range (adjust as needed)



// Timer Configuration
//   (claude.ai)
// Timer Hardware defines
#define TIMER_GROUP          TIMER_GROUP_0
#define TIMER_IDX            TIMER_0

// Timer ISR state machine states
typedef enum {
    STATE_GPIO_TOGGLE,
    STATE_SAMPLE_1,
    STATE_SAMPLE_2,
    STATE_SAMPLE_3,
} timer_state_t;

//  Compute timer config
// For 200 Hz square wave
#define SQUARE_WAVE_FREQ_HZ  200    // HZ  desired freq
#define TIMER_RESOLUTION_HZ  1000000  // 1MHz = 1µs resolution
// TIMER_BASE_CLK is 80 MHz for ESP32


// Data acquisition parameters
#define SAMPLES_PER_PHASE 3  // how many A/D samples to take each 1/2 cycle.
                            // (cant change this without changing ISR state machine.)
#define DAQ_DURATION          0.5 // sec  How long will we collect data for
#define SENSING_CYCLES_NUM    100 // number of excitation ON+OFF cycles per measurement
#define PHASE_DURATION_US    TIMER_RESOLUTION_HZ / (2*SQUARE_WAVE_FREQ_HZ)   // e.g. 1/2 cycle
#define SAMPLE_DELAY_US      PHASE_DURATION_US/2     // Wait 1/4 cycle before starting samples
#define INTER_SAMPLE_US      100      // 100µs between samples


// Data Buffer Storage
//
//  Cant do this math in c-preProcessor cmd, just compute it and type it in here
#define PHOTO_DATA_BUF_SIZE   600 // 2*SAMPLES_PER_PHASE * DAQ_DURATION * SQUARE_WAVE_FREQ_HZ

extern volatile uint16_t  data_buffer[PHOTO_DATA_BUF_SIZE];  // where data will be stored
extern volatile uint16_t *data_ptr;   // pointer for async writing/reading buff.

extern volatile uint8_t phase_buffer[PHOTO_DATA_BUF_SIZE];  // where phase tag will be stored
extern volatile uint8_t *phase_ptr;   // pointer for async writing/reading buff.

// each data value will be tagged as taken from the "excitation-on" phase,
// or the "excitation-off" phase.
#define EXCITATION_ON   1
#define EXCITATION_OFF  0

// Sync Semaphore
// Declare the semaphore handle as extern so other files can access it
extern SemaphoreHandle_t acquisition_complete_sem;

// Function to initialize the semaphore (call once during init)
void init_acquisition_semaphore(void);

// function declarations
esp_err_t init_photonics(void);  // initialize photonic_task
void photonic_task(void*);     // generate Ex signal and collect data
unsigned long int collect_PD_ADC(int);       // get an ADC reading from the PD amp.
unsigned long int photonic_test(void);       // test method for ADC

void start_timer(gptimer_handle_t);

// pin assignments for photonics


// globals
extern uint32_t sensing_cycle_count;
extern uint64_t next_alarm_count;
extern uint8_t  phase;

// Globals for ISR
extern gptimer_handle_t gptimer;
extern volatile timer_state_t isr_state;
extern volatile uint8_t gpio_level;

extern volatile uint16_t samples_positive[SAMPLES_PER_PHASE];
extern volatile uint16_t samples_zero[SAMPLES_PER_PHASE];

#endif  // prevent double includes
