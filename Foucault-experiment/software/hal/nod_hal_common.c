#include "nod_hal.h"

#include <stdint.h>

void nod_assert_(bool cond, char *file, int line, const char *cond_s)
{
    if (!cond)
    {
        while (true)
        {
            nod_printf("Assert failed: %s:%d: %s\n", file, line, cond_s);
            nod_time_sleep_sec(1.000);
        }
    }
}

double nod_time_get_sec(void)
{
    return nod_time_get_us() / 1000.0 / 1000.0;
}

nod_status_t nod_pwm_write_us(nod_pwm_pin_t pin, uint32_t freq, uint32_t duty_us)
{
    nod_printf("nod_pwm_write_us: pin=%d freq=%d duty_us=%d\n", pin, freq, duty_us);

    const uint32_t pulse_ms = duty_us / 1000;
    const uint32_t period_ms = 1000 / freq;
    const uint32_t duty_max = ((1 << /* resolution */ 20) - 1);

    const uint32_t duty =  pulse_ms / period_ms * duty_max;

    return nod_pwm_write_duty(pin, duty);
}
