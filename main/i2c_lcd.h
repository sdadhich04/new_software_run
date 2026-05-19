#ifndef I2C_LCD_H
#define I2C_LCD_H
#include "i2c_lcd.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "unistd.h"

// GPIO number used for I2C master clock
#define I2C_MASTER_SCL_IO           GPIO_NUM_4  // or 5/4????

// GPIO number used for I2C master data
#define I2C_MASTER_SDA_IO           GPIO_NUM_5

// I2C master port number (number of I2C peripheral interfaces available depends on the chip)
#define I2C_MASTER_NUM              0
// Define the I2C number to be used
#define I2C_NUM                     I2C_NUM_0


// I2C master clock frequency
#define I2C_MASTER_FREQ_HZ          400000

// I2C master doesn't need buffer for transmission
#define I2C_MASTER_TX_BUF_DISABLE   0

// I2C master doesn't need buffer for reception
#define I2C_MASTER_RX_BUF_DISABLE   0

// Timeout for I2C master in milliseconds
#define I2C_MASTER_TIMEOUT_MS       1000


// LCD command definitions
#define LCD_CMD_CLEAR_DISPLAY 0x01
#define LCD_CMD_RETURN_HOME 0x02
#define LCD_CMD_ENTRY_MODE_SET 0x06
#define LCD_CMD_DISPLAY_ON 0x0C
#define LCD_CMD_DISPLAY_OFF 0x08
#define LCD_CMD_FUNCTION_SET 0x28
#define LCD_CMD_SET_CURSOR 0x80
#define LCD_CMD_INIT_8_BIT_MODE 0x30
#define LCD_CMD_INIT_4_BIT_MODE 0x20

/** (BH Jul25)
 *  @brief initialize i2c as req'd for LCD 16xx02 w/ backpack
 *
 *   Originally included in lcd_init(), this fcn sets up the i2c Master
 *   parameters so we can abstract it for multiple devices.
 */
esp_err_t i2c_master_init(void);

/**
 * @brief Initializes the LCD
 *
 * This function sets up the LCD for use, initializing the required configurations.
 */
void lcd_init(uint8_t lcd_addr);

/**
 * @brief Sends a command to the LCD
 *
 * @param lcd_addr The address to the LCD
 *
 * @param cmd The command to send to the LCD
 *
 * This function sends a command to the LCD to perform various control operations.
 */
void lcd_send_cmd(uint8_t lcd_addr, char cmd);

/**
 * @brief Sends data to the LCD
 *
 * @param lcd_addr The address to the LCD
 *
 * @param data The data to send to the LCD
 *
 *
 * This function sends a data byte to the LCD, which is displayed on the screen.
 */
void lcd_send_data(uint8_t lcd_addr, char data);

/**
 * @brief Sends a string to the LCD
 *
 * @param str The string to send to the LCD
 *
 * This function sends a null-terminated string to the LCD to be displayed.
 */
void lcd_send_string(uint8_t lcd_addr, char *str);

/**
 * @brief Sets the cursor position on the LCD
 *
 * @param row The row number (0 or 1)
 * @param col The column nu
 * mber (0-15)
 *
 * This function positions the cursor on the LCD at the specified row and column.
 */
void lcd_put_cursor(uint8_t lcd_addr, int row, int col);

/**
 * @brief Clears the LCD screen
 *
 * This function clears all the content displayed on the LCD and resets the cursor position.
 */
void lcd_clear(uint8_t lcd_addr);

#endif /* I2C_LCD_H */
