/*
 * includes and defines for use of LCD_task.c
 *     BH Dec 25
 */
#include "esp_system.h"
#include "i2c_lcd.h"

#define SLAVE_ADDRESS1_LCD 0x27
#define SLAVE_ADDRESS2_LCD 0x26

void LCD_task1(void* argptr);
void LCD_task2(void* argptr);  // we're only using one in TPT-finder
void lcd_task_3(void* argptr);  // this one works with messageQueue
void lcd_task_3a(void* argptr);  // sends regular messages to the messageQueue
void lcd_message(uint8_t, uint8_t, char* );

esp_err_t LCD_16x2_init();

void LCD_16x2_task(void*);  // void* required by FreeRTOS

void LCD_reset(uint8_t);


// new messageQueue based LCD driver method
// 1. Define a message structure
typedef struct {
    uint8_t row;
    uint8_t col;
    char text[20];  // Adjust size as needed
} lcd_message_t;

extern QueueHandle_t lcdQueue;
