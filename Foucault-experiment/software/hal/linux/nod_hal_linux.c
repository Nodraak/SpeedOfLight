#include "nod_hal.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
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

double nod_time_get_sec(void)
{
    return nod_time_get_us() / 1000.0 / 1000.0;
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
    (void)pin; (void)freq;
    return NOD_STATUS_SUCCESS;
}

nod_status_t nod_pwm_write_duty(nod_pwm_pin_t pin, uint32_t duty)
{
    (void)pin; (void)duty;
    return NOD_STATUS_SUCCESS;
}

nod_status_t nod_pwm_write_us(nod_pwm_pin_t pin, uint32_t freq, uint32_t duty_us)
{
    (void)pin; (void)freq; (void)duty_us;
    return NOD_STATUS_SUCCESS;
}

/*
    ADC
*/

nod_status_t nod_adc1_init(nod_adc1_channel_t ch)
{
    (void)ch;
    return NOD_STATUS_SUCCESS;
}

int nod_adc1_read(nod_adc1_channel_t ch)
{
    (void)ch;
    return 512; // fake middle value
}

/*
    Timer
*/

nod_status_t nod_timer_init(nod_timer_t *timer, uint32_t frequency, uint64_t alarm_value, void (*userFunc)(void))
{
    (void)timer; (void)frequency; (void)alarm_value; (void)userFunc;
    return NOD_STATUS_SUCCESS;
}

/*
    Mutex
*/

void nod_mutex_init(nod_mutex_t *mutex)
{
    (void)mutex;
}

void nod_mutex_lock(nod_mutex_t *mutex)
{
    (void)mutex;
}

void nod_mutex_unlock(nod_mutex_t *mutex)
{
    (void)mutex;
}
