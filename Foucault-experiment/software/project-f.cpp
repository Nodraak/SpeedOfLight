#include "nod_hal.h"

#include <stdint.h>

// ----------------------- User config -----------------------

#define SERIAL_BAUD     115200

#define PWM_PIN         (nod_pwm_pin_t)32                   // GPIO32
#define ADC_PIN         (nod_adc1_channel_t)ADC1_CHANNEL_5  // GPIO33

#define PWM_FREQ_HZ     50
#define PWM_MIN_US      1000
#define PWM_MAX_US      2000

// TODO
#define SAMPLE_US       62   // ~16 kHz = 1 000 000 RPM
// 100 k RPM, 100 samples

#define VAL_HIST_BINS   256
#define IRQ_HIST_BINS   1024
#define REV_HIST_BINS   1024

#define PRINT_CYCLE_SEC 1.000

// ----------------------- Globals -----------------------

nod_mutex_t timer_mutex = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t sample_count = 0, crossing_count = 0;
volatile uint32_t val_hist[VAL_HIST_BINS] = {0};
volatile uint32_t irq_timing_hist[IRQ_HIST_BINS] = {0};
volatile uint32_t rev_timing_hist[REV_HIST_BINS] = {0};

uint32_t threshold_bin = 0;

// ----------------------- Utilities -----------------------

uint32_t nod_pulseUS_to_duty(uint32_t pulse_us)
{
    return (uint32_t)(
        (
            (uint64_t)pulse_us * ((1 << 16) - 1)
        ) / 20000UL
    );
}

uint32_t nod_clampRPM(uint32_t v)
{
    return (v < 1000) ? 1000 : (v > 100000) ? 100000 : v;
}

uint32_t nod_rpm_to_pulse_us(uint32_t rpm)
{
    return PWM_MIN_US + (uint32_t)(
        (
            (uint64_t)(nod_clampRPM(rpm) - 1000) * (PWM_MAX_US - PWM_MIN_US)
        ) / 99000
    );
}

// ----------------------- ESC Arming -----------------------

void nod_armESC(void)
{
    nod_printf("Arming ESC...\n");

    // Option A: standard low-high-low
    nod_pwm_write(PWM_PIN, nod_pulseUS_to_duty(PWM_MIN_US));
    nod_time_sleep_sec(2.000);
    nod_pwm_write(PWM_PIN, nod_pulseUS_to_duty(PWM_MAX_US));
    nod_time_sleep_sec(2.000);
    nod_pwm_write(PWM_PIN, nod_pulseUS_to_duty(PWM_MIN_US));
    nod_time_sleep_sec(2.000);
    nod_printf("ESC armed (standard sequence)\n");

    /*
    // Option B: simple safe-low only
    nod_pwm_write(PWM_PIN, nod_pulseUS_to_duty(PWM_MIN_US));
    nod_time_sleep_sec(3.000);
    nod_printf("ESC armed (safe-low only)\n");
    */
}

// ----------------------- ISR -----------------------

void IRAM_ATTR nod_timer_irq(void)
{
    static bool currently_above = false;

    const uint32_t start_sec = nod_time_get_sec();

    const int raw = adc1_get_raw(ADC_PIN);

    const uint32_t bin = (uint32_t)((raw*HIST_BINS)/4096);

    nod_mutex_lock_ISR(&timer_mutex);

    hist[bin]++;
    sample_count++;

    // crossing detection
    if (raw >= threshold_bin*16)
    {
        if (!currently_above)
        {
            crossing_count++;
            currently_above = true;
        }
    }
    else {
        currently_above = false;
    }

    // ISR timing profiling
    const uint32_t dur_sec = nod_time_get_sec() - start_sec;
    if (dur_sec >= 0.001 && dur_sec <= 1.000)
    {
        irq_timing_hist[duration/1000.0 - 1]++;
    }

    nod_mutex_unlock_ISR(&timer_mutex);
}

// ----------------------- Decile helper -----------------------

void nod_deciles(uint32_t *hist, int bins, int count, uint8_t *out)
{
    const uint32_t target = count / 10;
    uint32_t acc = 0, idx = 0;

    for (int d = 0; d < 10; d++)
    {
        while (idx < bins && acc < target)
        {
            acc += hist[idx++];
        }
        out[d] = idx-1;
    }
}

// ----------------------- Main -----------------------

int main(void)
{
    nod_stdout_init(SERIAL_BAUD);
    nod_time_sleep_sec(0.200);
    nod_printf("\nESP32 ESC+RPM+Profiler\n");

    nod_adc1_init(ADC_PIN);

    nod_pwm_init(PWM_PIN, PWM_FREQ_HZ);
    nod_pwm_write(PWM_PIN, nod_pulseUS_to_duty(PWM_MIN_US));

    for (int i = 0; i < VAL_HIST_BINS; i++)
    {
        val_hist[i] = 0;
    }

    // void timerAlarm(hw_timer_t * timer, uint64_t alarm_value, bool autoreload, uint64_t reload_count);
    // timer = timerBegin(1000000);
    // timerAttachInterrupt(timer, &onTimer);
    // timerAlarm(timer, 1000000, true, 0);

    nod_timer_t timer;
    nod_timer_init(&timer, /* frequency */ 1 * 1000 * 1000, /* alarm_value */ 100 * 1000 / 60 * 100 /* = 170 K */, &nod_timer_irq);

    nod_armESC();

    while (true)
    {
        static uint32_t last_print_sec = nod_time_get_sec();

        // Read serial input

        uint32_t rpm_command = 1000;
        static String buf = "";

        while (nod_stdin_peek())
        {
            const char c = nod_stdin_read();
            if (c == '\r')
            {
                continue;
            }

            if (c == '\n')
            {
                if (buf.length() > 0)
                {
                    rpm_command = nod_clampRPM(buf.toInt());
                    buf = "";
                    nod_printf("[CMD] RPM = %u\n", rpm_command);

                    // Update PWM
                    nod_pwm_write(PWM_PIN, nod_pulseUS_to_duty(nod_rpm_to_pulse_us(rpm_command)));
                }
            }
            else
            {
                buf += c;
                if (buf.length() > 16)
                {
                    buf = buf.substring(buf.length() - 16);
                }
            }
        }

        // 1 Hz status print

        const uint32_t now_sec = nod_time_get_sec();

        if (now_sec - last_print_sec >= PRINT_CYCLE_SEC)
        {
            uint32_t local_hist[VAL_HIST_BINS], local_count, local_cross;

            nod_mutex_lock(&timer_mutex);

            for (int i = 0; i < VAL_HIST_BINS; i++)
            {
                local_hist[i] = val_hist[i];
                val_hist[i] = 0;
            }

            local_count = sample_count;
            sample_count = 0;
            local_cross = crossing_count;
            crossing_count = 0;

            nod_mutex_unlock(&timer_mutex);

            // median threshold

            uint32_t acc = 0, median = 0;
            for (int i = 0; i < VAL_HIST_BINS; i++)
            {
                acc += local_hist[i];
                if (acc >= local_count/2)
                {
                    median = i;
                    break;
                }
            }

            threshold_bin = median;

            // RPM

            uint32_t measured_rpm = local_cross * 60;

            // ISR timing deciles

            uint32_t timing_snapshot[IRQ_HIST_BINS];
            uint8_t timing_dec[10];

            nod_mutex_lock(&timer_mutex);

            for (int i = 0;i < IRQ_HIST_BINS; i++)
            {
                timing_snapshot[i] = irq_timing_hist[i];
                irq_timing_hist[i] = 0;
            }

            nod_mutex_unlock(&timer_mutex);

            nod_deciles(timing_snapshot, IRQ_HIST_BINS, local_count, timing_dec);

            // Rotation duration histogram

            uint32_t rot_snapshot[REV_HIST_BINS];
            uint8_t rot_dec[10];

            nod_mutex_lock(&timer_mutex);

            for (int i = 0; i < REV_HIST_BINS; i++)
            {
                rot_snapshot[i] = rev_timing_hist[i];
                rev_timing_hist[i] = 0;
            }

            nod_mutex_unlock(&timer_mutex);

            // Print

            nod_printf(
                "[STAT] rpm_command = %u, measured_rpm = %u, samples = %u, threshold = %u, ISRdec = ",
                rpm_command, measured_rpm, local_count, threshold_bin
            );
            for (int i = 0; i < 10; i++)
            {
                nod_printf("%u%s", timing_dec[i], (i < 9) ? "," : "");
            }
            nod_printf("\n");

            last_print_sec += 1.000;
        }

        nod_time_sleep_sec(0.001);
    }

    return 0;
}
