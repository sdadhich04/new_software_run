/* University of Washington TPT-Finder Project
 * Blake Hannaford (Jul 25)
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "led_strip.h"
#include "esp_system.h"
#include "i2c_lcd.h"
#include "LCD_task.h"
#include "unistd.h"

extern SemaphoreHandle_t i2cMutex;
extern void handle_error(char* err_msg);

// for sprintf use
char err_msg[200];

// void LCD_16x2_task(void*);
// void LCD_16x2_init(void) ;
// void LCD_task(void*);

/*************************   LCD code
 *
 *
 */
#define d100ms   100000
#define d200ms   200000

/*
 *  use idf.py menuconfig
 *
 *   (select "BH_demo_Configuration2")
 */

int check_lcd_addr(uint8_t lcd_addr, char* cmsg){
    int valid = 0;
    if (lcd_addr==(uint8_t) SLAVE_ADDRESS1_LCD) valid = 1;
    if (lcd_addr==(uint8_t) SLAVE_ADDRESS2_LCD) valid = 2;
    if(valid==0){
        sprintf(err_msg, "illegal LCD address (i2c bus): 0x%x  [%s]", lcd_addr, cmsg);
        handle_error(err_msg);
        }
    return lcd_addr;
}

#ifdef  CONFIG_DRIVE_LCD_YES

#define LCD_tasks_TAG "LCD_Tasks:"

void LCD_reset(uint8_t lcd_addr) {
    ESP_LOGI(LCD_tasks_TAG, "LCD_Reset: 0x%x", lcd_addr);

    uint8_t lcda = check_lcd_addr(lcd_addr, "LCD_reset()");
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        printf("LCD_reset: Starting: 0x%x\n", lcd_addr);

        lcd_init(lcda);             // Initialize the LCD
        usleep(d100ms);

        lcd_clear(lcda);            // Clear the LCD screen

        usleep(d200ms);
        xSemaphoreGive(i2cMutex);
        usleep(d100ms);
        vTaskDelay(pdMS_TO_TICKS(100)); // NEEDED???
    }
}


void lcd_task_3(void* argptr){
    uint8_t lcd_addr;
    uint8_t lcda;
    lcd_message_t lcd_msg;
    ESP_LOGI(LCD_tasks_TAG, " .. got here");

    lcd_addr = *((uint8_t *)argptr);
    lcda   = check_lcd_addr(lcd_addr,"LCD_task startup");

    while (1) {
        // Wait for message (blocks until message arrives)
        if (xQueueReceive(lcdQueue, &lcd_msg, portMAX_DELAY) == pdTRUE) {
            // ESP_LOGI(LCD_tasks_TAG, "rcvd lcd_msg from queue (3)");
            if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
                lcd_put_cursor(lcda, lcd_msg.row, lcd_msg.col);
                lcd_send_string(lcda, lcd_msg.text);
                xSemaphoreGive(i2cMutex);
            }
        }
    }}

/*
    * a simple task to test LCD every x seconds
    */
void lcd_task_3a(void* argptr){
    lcd_message_t lcd_msg;
    int counter = 0;

    while (1) {
        // Prepare message
        lcd_msg.row = 1;
        lcd_msg.col = 0;
        snprintf(lcd_msg.text, sizeof(lcd_msg.text), "N: %04d", counter++);

        // Send to LCD task (non-blocking with timeout)
        if (xQueueSend(lcdQueue, &lcd_msg, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW("TASK", "LCD queue full!");
        }
        // ESP_LOGI(LCD_tasks_TAG, "Message sent from 3a");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void lcd_message(uint8_t row, uint8_t col, char* text){
    lcd_message_t lcd_msg;
    lcd_msg.row = row;
    lcd_msg.col = col;
    strcpy(lcd_msg.text, text);
    // Send to LCD task (non-blocking with timeout)
    if (xQueueSend(lcdQueue, &lcd_msg, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW("TASK", "LCD queue full!");
    }
    ESP_LOGI(LCD_tasks_TAG, "Message sent using lcd_msg()");

}


void LCD_task1(void* argptr) {
    ESP_LOGI(LCD_tasks_TAG, "LCD_task1 is starting up NOW");
    uint8_t lcd_addr = *((uint8_t *)argptr);
    uint8_t lcda = check_lcd_addr(lcd_addr,"LCD_task1 startup");
    int i=0;   // iteration counter
    char numst[20];  // place to hold string to print

    char buffer[20];
    lcd_put_cursor(lcda, 0, 0);   // Set cursor position to   row, column
    sprintf(buffer, "Works: 0x%x", lcda);
    lcd_send_string(lcda, buffer);
    ESP_LOGI(LCD_tasks_TAG, "LCD_task1 is starting its LOOP ");

    while (1) {
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
            // ESP_LOGI(LCD_tasks_TAG, "got the semaphore unlock i2c");

            /* We were able to obtain the semaphore and can now access the
//                 shared resource. */
            lcd_put_cursor(lcda, 1, 0);

            sprintf(numst, "N: %04d", i++);

            lcd_send_string(lcda, numst);     // Display the count (numst)
            //usleep(3*d100ms);

            /* We have finished accessing the shared resource. Release the
            semaphore. */
            xSemaphoreGive(i2cMutex);
            // ESP_LOGI(LCD_tasks_TAG, "released semaphore");

            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

/*
 *  LCD2 no longer supported
 *
void LCD_task2(void* argptr) {
    uint8_t lcd_addr = *((uint8_t *)argptr);
    uint8_t lcda = check_lcd_addr(lcd_addr,"LCD task2 startup");
    char buffer[20];

    sprintf(buffer, "LCD 0x%x works!", lcda);

    uint8_t flip = 0;
    ESP_LOGI(LCD_tasks_TAG, "LCD_task2() is starting up NOW");
    while (1) {
        if(xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
            lcd_clear(lcd_addr);
            lcd_put_cursor(lcd_addr, flip, 0);

            lcd_send_string(lcd_addr, buffer);
        }
        xSemaphoreGive(i2cMutex);
        flip = flip ^ 1;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
 */

#elif   CONFIG_DRIVE_LCD_NO
#define LCD_tasks_TAG  "NO LCD_16x2_task"

char lcd_LOG_message[] = "CONFIG:DRIVE_LCD:  NO_LCD ";

void LCD_16x2_task(void*){
    while(1){
        ESP_LOGI(LCD_tasks_TAG, "Not Configured in menuconfig: this is placeholder: LCD_16x2_task()");
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}


void LCD_16x2_init(void) {
    ESP_LOGI(LCD_tasks_TAG, "Not Configured in menuconfig: this is placeholder: LCD_16x2_init()");
}
#else
#error "unsupported LCD config"
#endif
