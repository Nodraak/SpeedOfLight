#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NOD_STATUS_SUCCESS = 0,
    NOD_STATUS_ERROR   = 1,
} nod_status_t;

/*
    Time
*/

// Arduino ESP32: delay()
// Linux: usleep()
void nod_time_sleep_sec(double delay_sec);

// Arduino ESP32: esp_timer_get_time() (returns us) (alternative: millis())
// Linux: clock_gettime()
double nod_time_get_sec();

/*
    Stdio
*/

// Arduino ESP32: Serial().begin()
void nod_stdout_init(uint32_t baudrate);

// Arduino ESP32: Serial().println()
void nod_printf(const char *fmt, ...);

// Arduino ESP32: Serial().available()
int nod_stdin_peek(void);

// Arduino ESP32: Serial().read()
int nod_stdin_read(void);

/*
    PWM (LEDC)
    https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html
*/

enum nod_pwm_pin_t : int;
typedef enum nod_pwm_pin_t nod_pwm_pin_t;

// bool ledcSetClockSource(ledc_clk_cfg_t source);
//      source:
//          LEDC_APB_CLK - APB clock: 80 MHz (?)
//          LEDC_REF_CLK - REF clock: 1 MHz (?). Hardcoded to this clock.
// bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution);
//      resolution: range is 1-14 bits (1-20 bits for ESP32). Hardcoded to 20.
// Linux: no op
nod_status_t nod_pwm_init(nod_pwm_pin_t pin, uint32_t freq);

// bool ledcWrite(uint8_t pin, uint32_t duty);
// Linux: no op
nod_status_t nod_pwm_write(nod_pwm_pin_t pin, uint32_t duty);

/*
    ADC
    https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/peripherals/adc.html

    Arduino/ESP32 notes:

    (Target: 100 k RPM = 170 k samples/sec)

    * Simple Arduino analogRead() / naive loop: 0.1-1 k samples/sec
    * adc1_get_raw() in a tight ISR or loop: 10-100 k samples/sec
        * single ADC1 channel in optimized code
    * I2S / ADC DMA mode (continuous sampling with DMA): 100-1000 k samples/sec
        * Note: some ESP-IDF docs / driver modes document lower practical limits (e.g. certain I2S-ADC built-in modes
            note 150 kHz as a safe target for some configurations)

    Alternative (need digitalization frontend): PCNT
    https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/pcnt.html
*/

enum nod_adc1_channel_t : int;
typedef enum nod_adc1_channel_t nod_adc1_channel_t;

// esp_err_t adc1_config_width(adc_bits_width_t width_bit)
//      width_bit: hardcoded to ADC_WIDTH_BIT_12
// esp_err_t adc1_config_channel_atten(adc1_channel_t channel, adc_atten_t atten)
//      atten: hardcoded to ADC_ATTEN_DB_11
// Linux: no op
nod_status_t nod_adc1_init(nod_adc1_channel_t pin);

// Arduino ESP32: int adc1_get_raw(adc1_channel_t channel)
// Linux: no op
int nod_adc1_read(nod_adc1_channel_t pin);

/*
    Timer
    https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html
*/

typedef struct nod_timer_t nod_timer_t;

// Arduino ESP32: hw_timer_t * timerBegin(uint32_t frequency);
// Arduino ESP32: void timerAttachInterrupt(hw_timer_t * timer, void (*userFunc)(void));
// Arduino ESP32: void timerAlarm(hw_timer_t * timer, uint64_t alarm_value, bool autoreload, uint64_t reload_count);
nod_status_t nod_timer_init(nod_timer_t *timer, uint32_t frequency, uint64_t alarm_value, void (*userFunc)(void));

/*
    Mutex
*/

// Arduino ESP32: portMUX_TYPE isrMux
typedef struct nod_mutex_t nod_mutex_t;

// Arduino ESP32: portENTER_CRITICAL_ISR()
// Linux: no op
void nod_mutex_lock(nod_mutex_t *mutex);

// Arduino ESP32: portEXIT_CRITICAL_ISR()
// Linux: no op
void nod_mutex_unlock(nod_mutex_t *mutex);
