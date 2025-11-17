#include "nod_hal.h"

#include <Arduino.h>
#include "driver/adc.h"
#include "driver/ledc.h"
#include "esp_timer.h"

#include <stdarg.h>
#include <stdio.h>

/*
    Main
*/

// run once, should not return
int main(void);

// Arduino entry point 1/2
void setup(void)
{
    // run once, should not return
    main();
}

// Arduino entry point 2/2
void loop(void)
{
    while (true)
    {
        nod_printf(".\n");
        nod_time_sleep_sec(1.000);
    }
}

/*
    Time
*/

void nod_time_init(void)
{
    // Nothing to do
}

void nod_time_sleep_sec(double delay_sec)
{
    delay((uint32_t)(delay_sec * 1000.0));
}

uint32_t nod_time_get_us(void)
{
    return esp_timer_get_time();
}

/*
    Stdio
*/

void nod_stdout_init(uint32_t baudrate)
{
    Serial.begin(baudrate);
}

void nod_printf(const char *fmt, ...)
{
    char buf[256] = "";
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Serial.print(buf);
}

int nod_stdin_peek(void)
{
    return Serial.available();
}

int nod_stdin_read(void)
{
    if (Serial.available())
        return Serial.read();
    return -1;
}

/*
    PWM (LEDC)
*/

nod_status_t nod_pwm_init(nod_pwm_pin_t pin, uint32_t freq)
{
    bool ret = true;

    ret = ledcSetClockSource(LEDC_APB_CLK);
    if (ret == true)
    {
        return NOD_STATUS_SUCCESS;
    }
    else
    {
        return NOD_STATUS_ERROR;
    }

    ret = ledcAttach((uint8_t)pin, freq, /* resolution */ 20);
    if (ret == true)
    {
        return NOD_STATUS_SUCCESS;
    }
    else
    {
        return NOD_STATUS_ERROR;
    }
}

nod_status_t nod_pwm_write_duty(nod_pwm_pin_t pin, uint32_t duty)
{
    const bool ret = ledcWrite((uint8_t)pin, duty);
    if (ret == true)
    {
        return NOD_STATUS_SUCCESS;
    }
    else
    {
        return NOD_STATUS_ERROR;
    }
}

/*
    ADC
*/

nod_status_t nod_adc1_init(nod_adc1_channel_t ch)
{
    const esp_err_t ret = ESP_OK;

    ret = adc1_config_width(ADC_WIDTH_BIT_12);
    if (ret != ESP_OK)
    {
        return NOD_STATUS_ERROR;
    }

    ret = adc1_config_channel_atten((adc1_channel_t)ch, ADC_ATTEN_DB_11);
    if (ret != ESP_OK)
    {
        return NOD_STATUS_ERROR;
    }

    return NOD_STATUS_SUCCESS;
}

int nod_adc1_read(nod_adc1_channel_t ch)
{
    return adc1_get_raw((adc1_channel_t)ch);
}

/*
    Timer
*/

nod_status_t nod_timer_init(nod_timer_t *timer, uint32_t frequency, uint64_t alarm_value, void (*userFunc)(void))
{
    timer->data = timerBegin(frequency);
    timerAttachInterrupt(timer->data, userFunc);
    timerAlarm(timer->data, alarm_value, /* autoreload */ true, /* reload_count */ 0);
    return NOD_STATUS_SUCCESS;
}

/*
    Mutex
*/

void nod_mutex_init(nod_mutex_t *mutex)
{
    mutex->data = portMUX_INITIALIZER_UNLOCKED;
}

void nod_mutex_lock(nod_mutex_t *mutex)
{
    portENTER_CRITICAL_ISR(&mutex->data);
}

void nod_mutex_unlock(nod_mutex_t *mutex)
{
    portEXIT_CRITICAL_ISR(&mutex->data);
}
