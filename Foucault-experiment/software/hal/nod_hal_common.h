#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NOD_STATUS_SUCCESS = 0,
    NOD_STATUS_ERROR   = 1,
} nod_status_t;

/*
    Assert
*/

// public API
void nod_assert(bool cond);

// private implementation
#define nod_assert(cond) nod_assert_(cond, __FILE__, __LINE__, #cond)
void nod_assert_(bool cond, char *file, int line, const char *cond_s);

/*
    Time
*/

void nod_time_init(void);

// Arduino ESP32: delay()
// Linux: usleep()
void nod_time_sleep_sec(double delay_sec);

// Arduino ESP32: esp_timer_get_time() (returns us) (alternative: millis())
// Linux: clock_gettime()
uint32_t nod_time_get_us(void);

double nod_time_get_sec(void);

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

typedef enum {
    NOD_PWM_GPIO_0 = 0, // GPIO_NUM_0
    NOD_PWM_GPIO_32 = 32, // GPIO_NUM_32
    // TODO
} nod_pwm_pin_t;

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
nod_status_t nod_pwm_write_duty(nod_pwm_pin_t pin, uint32_t duty);

nod_status_t nod_pwm_write_us(nod_pwm_pin_t pin, uint32_t freq, uint32_t duty_us);

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

    Alternative (need digitalization frontend): GPIO IRQ or PCNT
    https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/pcnt.html
*/

typedef enum {
    NOD_ADC1_GPIO_36 = 0, // ADC1_CHANNEL_0
    NOD_ADC1_GPIO_33 = 5, // ADC1_CHANNEL_5
    // TODO
} nod_adc1_channel_t;

// esp_err_t adc1_config_width(adc_bits_width_t width_bit)
//      width_bit: hardcoded to ADC_WIDTH_BIT_12
// esp_err_t adc1_config_channel_atten(adc1_channel_t channel, adc_atten_t atten)
//      atten: hardcoded to ADC_ATTEN_DB_11
//          11 dB attenuation (ADC_ATTEN_DB_11) gives full-scale voltage 3.9 V
//          The maximum reading is 4095 for 12-bits
// Linux: no op
nod_status_t nod_adc1_init(nod_adc1_channel_t pin);

// Arduino ESP32: int adc1_get_raw(adc1_channel_t channel)
// Linux: no op
// Range: 0-4095 (resolution hardcoded to 12 bits)
int nod_adc1_read(nod_adc1_channel_t pin);

/*
    Timer
    https://docs.espressif.com/projects/arduino-esp32/en/latest/api/timer.html
*/

// Arduino ESP32: hw_timer_t
// Linux: empty
typedef struct nod_timer_t nod_timer_t;

// Arduino ESP32: hw_timer_t * timerBegin(uint32_t frequency);
// Arduino ESP32: void timerAttachInterrupt(hw_timer_t * timer, void (*userFunc)(void));
// Arduino ESP32: void timerAlarm(hw_timer_t * timer, uint64_t alarm_value, bool autoreload, uint64_t reload_count);
nod_status_t nod_timer_init(nod_timer_t *timer, uint32_t frequency, uint64_t alarm_value, void (*userFunc)(void));

/*
    Mutex
*/

// Arduino ESP32: portMUX_TYPE
// Linux: empty
typedef struct nod_mutex_t nod_mutex_t;

// Arduino ESP32: set to portMUX_INITIALIZER_UNLOCKED
// Linux: no op
void nod_mutex_init(nod_mutex_t *mutex);

// Arduino ESP32: portENTER_CRITICAL_ISR()
// Linux: no op
void nod_mutex_lock(nod_mutex_t *mutex);

// Arduino ESP32: portEXIT_CRITICAL_ISR()
// Linux: no op
void nod_mutex_unlock(nod_mutex_t *mutex);
