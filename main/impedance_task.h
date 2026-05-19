#ifndef IMPEDANCE_TASK_H //prevent double includes
#define IMPEDANCE_TASK_H

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "driver/usb_serial_jtag.h"
#include "AD5933.h"
// #include "LogWriter.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "led_strip.h"

#define I2C_MASTER_SCL_IO 20  // for new esp32c6 board
#define I2C_MASTER_SDA_IO 21 


#define MHz_6 6000000
#define INTERNAL_CLOCK_FREQ 16000000

#define START_FREQ 33000
#define NUM_INCR 1
#define FREQ_INCR 1000

#define CALIBRATION_NUM_INCR 3

#define BUFFER_SIZE 1024

#define IMPEDANCE_THRESHOLD 700


void init_impedance();
void impedance_task();


#endif  // prevent double includes
