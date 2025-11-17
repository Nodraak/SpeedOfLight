#include "nod_hal.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/*
    Time
*/

static struct timespec start_time;

void nod_time_init(void)
{
    clock_gettime(CLOCK_MONOTONIC, &start_time);
}

void nod_time_sleep_sec(double delay_sec)
{
    usleep((useconds_t)(delay_sec * 1e6));
}

uint32_t nod_time_get_us(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (
        (now.tv_sec - start_time.tv_sec) * 1000 * 1000
        + (now.tv_nsec - start_time.tv_nsec) / 1000
    );
}

/*
    Stdio
*/

void nod_stdout_init(uint32_t baudrate)
{
    (void)baudrate;
}

void nod_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

int nod_stdin_peek(void)
{
    // TODO
    // return getchar();  // simple blocking version
    return false;
}

int nod_stdin_read(void)
{
    return getchar();
}

/*
    PWM (LEDC)
*/

nod_status_t nod_pwm_init(nod_pwm_pin_t pin, uint32_t freq)
{
    nod_printf("nod_pwm_init: pin=%d freq=%d\n", pin, freq);
    return NOD_STATUS_SUCCESS;
}

nod_status_t nod_pwm_write_duty(nod_pwm_pin_t pin, uint32_t duty)
{
    nod_printf("nod_pwm_write_duty: pin=%d duty=%d\n", pin, duty);
    return NOD_STATUS_SUCCESS;
}

/*
    ADC
*/

nod_status_t nod_adc1_init(nod_adc1_channel_t ch)
{
    nod_printf("nod_adc1_init: ch=%d\n", ch);
    return NOD_STATUS_SUCCESS;
}

int nod_adc1_read(nod_adc1_channel_t ch)
{
    static FILE *f = NULL;

    (void)ch;
    // nod_printf("nod_adc1_read: ch=%d\n", ch);

    if (f == NULL)
    {
        f = fopen(NOD_ADC_STUB_FILEPATH, "r");
        return 0;
    }

    char buffer[64] = "";
    char *ret = fgets(buffer, 64, f);
    if (ret == NULL)
    {
        fclose(f);
        f = NULL;
        return 0;
    }

    return atoi(buffer);
}

/*
    Timer
*/

nod_status_t nod_timer_init(nod_timer_t *timer, uint32_t frequency, uint64_t alarm_value, void (*userFunc)(void))
{
    (void)timer; (void)userFunc;
    nod_printf("nod_timer_init: frequency=%d alarm_value=%d\n", frequency, alarm_value);
    return NOD_STATUS_SUCCESS;
}

/*
    Mutex
*/

void nod_mutex_init(nod_mutex_t *mutex)
{
    (void)mutex;
    nod_printf("nod_mutex_init\n");
}

void nod_mutex_lock(nod_mutex_t *mutex)
{
    (void)mutex;
    // nod_printf("nod_mutex_lock\n");
}

void nod_mutex_unlock(nod_mutex_t *mutex)
{
    (void)mutex;
    // nod_printf("nod_mutex_unlock\n");
}
