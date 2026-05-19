#include "i2c_lcd.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "unistd.h"


/*
 *   Modification (BH Jul25):
 *       Abstracting out i2c initialization so that it
 *       can be used by multiple devices on same i2c bus
 *
 */

esp_err_t err; // Variable to store I2C communication errors
static const char *TAG = "LCD"; // Tag for logging

static int i2cMasterinit = 0;  // flag to show i2c is setup

// I2C master initialization
esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER, // Set as I2C master mode
        .sda_io_num = I2C_MASTER_SDA_IO, // GPIO number for I2C SDA
        .scl_io_num = I2C_MASTER_SCL_IO, // GPIO number for I2C SCL
        .sda_pullup_en = GPIO_PULLUP_ENABLE, // Enable pull-up on SDA line
        .scl_pullup_en = GPIO_PULLUP_ENABLE, // Enable pull-up on SCL line
        .master.clk_speed = I2C_MASTER_FREQ_HZ, // Set I2C clock frequency
    };

    // Configure I2C parameters with the given configuration
    i2c_param_config(i2c_master_port, &conf);

    i2cMasterinit = 1;  // we are set up
    // Install the I2C driver with the specified parameters
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void lcd_send_cmd(uint8_t lcd_addr, char cmd)
{
    char data_u, data_l;
    uint8_t data_t[4];
    // ESP_LOGI(TAG, "LCD send_cmd addr: 0x%x  data: 0x%x", lcd_addr, cmd);

    data_u = (cmd & 0xf0); // Upper nibble of the command
    data_l = ((cmd << 4) & 0xf0); // Lower nibble of the command
    
    data_t[0] = data_u | 0x0C; // Enable (EN) = 1, Register Select (RS) = 0
    data_t[1] = data_u | 0x08; // Enable (EN) = 0, Register Select (RS) = 0
    data_t[2] = data_l | 0x0C; // Enable (EN) = 1, Register Select (RS) = 0
    data_t[3] = data_l | 0x08; // Enable (EN) = 0, Register Select (RS) = 0
    
    // Write data to the I2C device
    err = i2c_master_write_to_device(I2C_NUM, lcd_addr, data_t, 4, 1000);
    
    // Log an error message if there is an error in sending the command
    if (err != 0) ESP_LOGI(TAG, "Error in sending command");
}

void lcd_send_data(uint8_t lcd_addr, char data)
{
    // ESP_LOGI(TAG, "LCD send_data addr: 0x%x  data: 0x%x", lcd_addr, data);
    char data_u, data_l;
    uint8_t data_t[4];
    
    data_u = (data & 0xf0); // Upper nibble of the data
    data_l = ((data << 4) & 0xf0); // Lower nibble of the data
    
    data_t[0] = data_u | 0x0D; // Enable (EN) = 1, Register Select (RS) = 1
    data_t[1] = data_u | 0x09; // Enable (EN) = 0, Register Select (RS) = 1
    data_t[2] = data_l | 0x0D; // Enable (EN) = 1, Register Select (RS) = 1
    data_t[3] = data_l | 0x09; // Enable (EN) = 0, Register Select (RS) = 1
    
    // Write data to the I2C device
    err = i2c_master_write_to_device(I2C_NUM, lcd_addr, data_t, 4, 1000);
    
    // Log an error message if there is an error in sending the data
    if (err != 0) ESP_LOGI(TAG, "Error in sending data");
}

void lcd_clear(uint8_t lcd_addr)
{
    lcd_send_cmd(lcd_addr, LCD_CMD_CLEAR_DISPLAY); // Clear display command
    usleep(5000); // Wait for the command to execute
}

void lcd_put_cursor(uint8_t lcd_addr, int row, int col)
{
    switch (row)
    {
        case 0:
            col |= LCD_CMD_SET_CURSOR; // Set position for row 0
            break;
        case 1:
            col |= (LCD_CMD_SET_CURSOR | 0x40); // Set position for row 1
            break;
    }

    lcd_send_cmd(lcd_addr, col); // Send command to set cursor position
}

void lcd_init(uint8_t lcd_addr)
{
    // NEW:   i2c must be initialized first
    //    i2c_master_init(); // Initialize I2C master interface
    if (! i2cMasterinit) {
        ESP_LOGI(TAG, " i2c master is not set up before lcd_init() call.");
        return;
        }

    // 4-bit initialization sequence
    usleep(50000); // Wait for >40ms
    lcd_send_cmd(lcd_addr, LCD_CMD_INIT_8_BIT_MODE);
    usleep(5000);  // Wait for >4.1ms
    lcd_send_cmd(lcd_addr, LCD_CMD_INIT_8_BIT_MODE);
    usleep(200);  // Wait for >100us
    lcd_send_cmd(lcd_addr, LCD_CMD_INIT_8_BIT_MODE);
    usleep(10000);
    lcd_send_cmd(lcd_addr, LCD_CMD_INIT_4_BIT_MODE);  // Set 4-bit mode
    usleep(10000);

    // Display initialization
    lcd_send_cmd(lcd_addr, LCD_CMD_FUNCTION_SET); // Function set: 4-bit mode, 2-line display, 5x8 characters
    usleep(1000);
    lcd_send_cmd(lcd_addr, LCD_CMD_DISPLAY_OFF); // Display off
    usleep(1000);
    lcd_send_cmd(lcd_addr, LCD_CMD_CLEAR_DISPLAY);  // Clear display
    usleep(1000);
    lcd_send_cmd(lcd_addr, LCD_CMD_ENTRY_MODE_SET); // Entry mode set: increment cursor, no shift
    usleep(1000);
    lcd_send_cmd(lcd_addr, LCD_CMD_DISPLAY_ON); // Display on, cursor off, blink off
    usleep(1000);
}

void lcd_send_string(uint8_t lcd_addr, char *str)
{
    while (*str) lcd_send_data(lcd_addr, *str++); // Send each character of the string
}
