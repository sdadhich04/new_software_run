
idf_component_register(SRCS main.c i2c_lcd.c LCD_task.c timer_Photo.c state_machine.c impedance_task.h impedance_task.c AD5933.c AD5933.h
                    PRIV_REQUIRES driver spi_flash esp_driver_gpio esp_timer esp_adc
                    INCLUDE_DIRS ".")
