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

CPU: Xtensa dual-core 32-bit LX6 microprocessor, 240 MHz
Memory: 520 KiB RAM
ADC: 2 × 12-bit SAR ADCs, up to 18 channels, with four levels of attenuation

https://github.com/tombenke/darduino

* Select Tools > Board > Boards Manager from the Arduino IDE menus to open the "Boards Manager" view in the left side panel.
* Scroll down through the list of boards platforms until you see the "esp32 by Espressif Systems" entry.
* Click the "INSTALL" button at the bottom of the entry.
* Wait for the installation to finish.

```
I need a C code to run on an azdelivery esp32 devkitc v4, via the Arduino IDE, with the following functions:

* Control the motor/ESC
    * Input from stdin: read stdin, parse an int between 1 000 and 100 000 (RPM)
    * Create the PWM:
        * Digital output
        * (50 Hz: 20 ms idle, 1-2 ms signal)
        * Precision: 1 % (10 us = 2 400 clock ticks at 240 MHz)
            * Can we implement it in the main loop? Do we need to use IRQs? Are they timers or builtin PWM controller on the ESP32?
    * Dont forget the arming sequence of the ESC
* Measure the actual RPM
    * Read the diode signal (analog input) and detect discontinuity in the signal to detect one rotation
        * In oder to do that in a reliable way, we can use an adaptive threshold:
        * Prepare an array of say, 256 values initialised to 0
        * Each time a value is read, increment the corresponding cell
        * After a 1 second cycle, find out the median value
        * Use this median value as the threshold for the next 1 sec cycle
        * Keep track of when we are in or out of reading a signal, to not count multiple times a given signal (wait for it to go back down below the threshold)
    * Measure per-sample timing (ISR execution time profiling) with a 1024 array (histogram), with 1 us per cell (1-1000 us)
        * Print at 1 Hz statistics: 10 deciles values
    * Also measure rotation duration with a 1024 items array (scale it so that index 512 corresponds to the expect duration at the currently configured ESC RPM, and index 0 and 1023 correspond to x1/10 and x10 this value)
    * Precision: 100 k RPM, 100 samples / rotation = 170 000 samples/sec = 6 us / sample
        * (At 240 MHz, that's 15 000 clock ticks / sample)
        * Same question again: can we implement it in the main loop? Do we need to use IRQs? Are they timers or builtin PWM controller on the ESP32?
    * Print the result at 1 Hz: RPM count, the threshold value, and other statistics as needed
* Use pin 32 for the PWM and 33 for the ADC input
```

ADC notes:

(100 k RPM = 170 k samples/sec)

* Simple Arduino analogRead() / naive loop - 100-1000 samples/sec
* adc1_get_raw() in a tight ISR or loop - 10-100 k samples/sec
    * single ADC1 channel in optimized code
* I2S / ADC DMA mode (continuous sampling with DMA) - 100-1000 k samples/sec
    * Note: some ESP-IDF docs / driver modes document lower practical limits (e.g. certain I2S-ADC built-in modes note 150 kHz as a safe target for some configurations)

https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html
https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/timer.html

https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/pcnt.html
