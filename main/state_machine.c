#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "LCD_task.h"
#include "timer_Photo.h"
#include "state_machine.h"

#define TAG "State Machine Task: "

void state_machine_init(){
    // sets up timer, ADC ports, excitation outputs
    init_photonics();
    ESP_LOGI(TAG, "photonics pinouts have been set (via State Machine init.)");




    //Initialize the semaphore which syncs w/ the ISR
    init_acquisition_semaphore();

    // Reset to clean state first
    gpio_reset_pin(END_PAUSE_INPUT);

    // Configure step-by-step (this is what worked in the test)
    gpio_set_direction(END_PAUSE_INPUT, GPIO_MODE_INPUT);
    gpio_set_pull_mode(END_PAUSE_INPUT, GPIO_PULLUP_ONLY);

    // Small delay to let it stabilize
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "End Pause Pin Configured");



    // set up GPIO
    ESP_LOGI(TAG, "State Machine input pin has been set (via State Machine init.)");

    return;
}

/*
// State Machine Task  states (for reference, see state_machine.h)
typedef enum {
    SM_State_Paused,
    SM_State_Acquiring,
    SM_State_Uploading
} SM_state_t;
*/

static SM_state_t state=SM_State_Paused;


void state_machine_task(void *pvParameters){

    while (1)
    {
        int bit=1;
        ESP_LOGI(TAG, "StateMachine woke up.");
        bit = gpio_get_level(END_PAUSE_INPUT);
        // ESP_LOGI(TAG, "current button input... (%d)",bit);
        switch(state) {
            case SM_State_Paused:{
                ESP_LOGI(TAG, "*****SM_State_Paused");
                lcd_message(0,0,"                ");
                lcd_message(0,0,"Paused");
                // wait for user input via pushbutton
                while ((bit = gpio_get_level(END_PAUSE_INPUT)) == FLOATING_PIN){
                    vTaskDelay(pdMS_TO_TICKS(10));
                    // if ((j++)%100 == 0)
                        // ESP_LOGI(TAG, "waiting for button input... (%d)",bit);
                    }
                state = SM_State_Acquiring;
                break;
            }
            case SM_State_Acquiring:{
                ESP_LOGI(TAG, "*****SM_State_Acquiring");
                lcd_message(0,0,"Acquire");

                // STOP timer first if it's running
                gptimer_stop(gptimer);

                // initialize data pointers
                data_ptr = data_buffer;
                phase_ptr = phase_buffer;
                sensing_cycle_count = 0;
                // set up initial state of the ISR
                gpio_level = 0;           // req'd at start of DAQ cycles
                isr_state = STATE_GPIO_TOGGLE;  //set where ISR will start

                // Start the timer-->isr going for excitation and acquisition
                // Set next alarm
                gptimer_alarm_config_t alarm_config = {
                    .alarm_count = 2500,   //
                    .flags.auto_reload_on_alarm = false,
                };
                gptimer_set_alarm_action(gptimer, &alarm_config);
                start_timer(gptimer);

                // Wait for ISR to finish up and stop itself
                if (xSemaphoreTake(acquisition_complete_sem, pdMS_TO_TICKS(2000)) == pdTRUE) {
                        ESP_LOGI(TAG, "Acquisition completed successfully");
                    } else {
                        ESP_LOGW(TAG, "Acquisition timeout - may be incomplete!");
                    }
                state = SM_State_Uploading;
                break;
            }
            case SM_State_Uploading:{
                ESP_LOGI(TAG, "*****SM_State_Uploading");
                lcd_message(0,0,"Uploading");

                // transfer data, format it, and print it as .csv
                printf("\n\n   >>>START_LOG<<<       Download .csv\n");
                printf("j, tag, value\n");
                int j = 0;
                int value = 0;
                char tag[10];
                data_ptr = data_buffer;
                phase_ptr = phase_buffer;
                // int Ndata = PHOTO_DATA_BUF_SIZE-SAMPLES_PER_PHASE; // correcting fencepost prob
                int Ndata = PHOTO_DATA_BUF_SIZE; // correcting fencepost prob
                while(j < Ndata){
    // #define DEBUG_SIZE  20
                // while(j < DEBUG_SIZE){
                    if (*phase_ptr == EXCITATION_OFF){
                        strcpy(tag,"off");
                    }
                    else
                        strcpy(tag,"on ");
                    value = (int) *data_ptr;
                    printf("%3d, %s, %d\n",j, tag,value);
                    j++;
                    data_ptr++;
                    phase_ptr++;
                    // ESP_LOGI(TAG, "sample %d: %d ",j, (int) data_ptr);
                    vTaskDelay(2); // ticks
                }
                printf(">>>END_LOG<<<      End of .csv   \n\n");
                vTaskDelay(pdMS_TO_TICKS(50));
                state = SM_State_Paused;
                break;
            }
        }// end switch cases
    }
}

