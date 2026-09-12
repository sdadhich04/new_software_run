# ESP32 Impedance Monitor Prototype

ESP-IDF firmware and a small Python GUI for a UW TPT-Finder bio-impedance / photonic sensing prototype.

The firmware targets an ESP32-C6 project named `BH_demo_esp32c6`. In the current `main.c` configuration it initializes an I2C bus, registers an AD5933 impedance-converter device, calibrates against a 2200 ohm reference, and starts a FreeRTOS impedance task that logs impedance magnitude, phase, and compensated real/imaginary values over serial. It also runs the onboard LED blink task and a serial "Hello world" task.

The repo also contains code for optional pieces that are currently compile-time disabled in `main.c`: LCD output, a photonic LED/photodiode acquisition task, and a state machine that prints tagged acquisition samples as CSV-formatted serial output.

## Hardware And Tools

- ESP32-C6 target, with comments referencing the Waveshare ESP32-C6 Zero board.
- AD5933 impedance converter on I2C address `0x0D`.
- I2C pins used for the AD5933 in `main.c`: SCL GPIO 20 and SDA GPIO 21.
- Addressable onboard LED on GPIO 8 when using the LED-strip/RMT configuration.
- Optional photonic acquisition code: excitation output on GPIO 0 and ADC input on GPIO 2.
- Optional I2C LCD support for addresses `0x27` and `0x26`.
- ESP-IDF 5.5.0, target `esp32c6`, with `espressif/led_strip` 2.5.5 recorded in `dependencies.lock`.
- Python GUI dependencies visible in `gui.py`: PyQt5, pyqtgraph, pyserial, and `brl_data`.

## Running The Firmware

Use an ESP-IDF shell with the ESP32-C6 toolchain available:

```sh
idf.py set-target esp32c6
idf.py menuconfig
idf.py build
idf.py flash monitor
```

`menuconfig` exposes options for LCD support, LED type, LED-strip backend, blink GPIO, and blink period.

## Running The GUI

The GUI reads serial output from `/dev/ttyACM0`, looks for lines containing `impedance magnitude:` and `Calculated phase:`, plots impedance values, and records averaged impedance/phase samples through `brl_data`.

```sh
python gui.py
```

The `brl_data` package is imported from an absolute local path in `gui.py` and is not included in this repository. Adjust the serial port and package path before running on another machine.

The GUI includes threshold-based labels for "Chicken", "Pork", and "Unknown"; those labels are simple impedance thresholds in the current code, not a validated classifier.

## Credits

Git history in this checkout shows commits by `sdadhich04 <sdadhich@uw.edu>`. Source comments credit the University of Washington TPT-Finder project and Blake Hannaford for the FreeRTOS / TPT-Finder base code. Several comments also note Espressif sample-code origins and Claude assistance for timer/ISR-related code.
