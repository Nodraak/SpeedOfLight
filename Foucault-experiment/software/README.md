# Software

## Introduction

Functions:

* Control the motor/ESC
    * Input from stdin: 1-100 k RPM
    * Create the PWM
        * 50 Hz: 20 ms idle, 1-2 ms signal
        * Precision: 1 % (10 us = 2 400 clock ticks at 240 MHz)
* Measure the actual RPM
    * Read the diode (analog) and detect discontinuity in the signal
    * 100 k RPM, 100 samples / rotation = 16 000 samples/sec = 60 us / sample
    * At 240 MHz, that's 15 000 clock ticks / sample
    * Print result at 1 Hz: RPM count, min/max duration between two discontinuities (for quality check)


## Arduino IDE for ESP32

azdelivery esp32 devkitc v4

https://github.com/tombenke/darduino

* Select Tools > Board > Boards Manager from the Arduino IDE menus to open the "Boards Manager" view in the left side panel.
* Scroll down through the list of boards platforms until you see the "esp32 by Espressif Systems" entry.
* Click the "INSTALL" button at the bottom of the entry.
* Wait for the installation to finish.

