#include "nod_hal.h"
#include "nod_stats.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ----------------------- User config -----------------------

#define SERIAL_BAUD     115200

#define PWM_PIN         NOD_PWM_GPIO_32
#define ADC_PIN         NOD_ADC1_GPIO_33

#define PWM_FREQ_HZ     50
#define PWM_MIN_US      1000
#define PWM_MAX_US      2000

#define TIMER_FREQUENCY (1000 * 1000)
#define SAMPLES_PER_REV 1000

#define REV_MIN_RPM     100
#define REV_MAX_RPM     (1000*1000) // ADC 4 k RPM ; GPIO IRQ 1200 k RPM

// ADC with continuous sampling and threshold:
//   Angular resolution needed:
//   1 part per pi*D ; D = 200 mm => 1 part per 600
//   => Aim for 1/1000
//   Req for 100 k RPM * 1000 samples/rev => 1.5 M samples/sec = 0.7 us/sample
//   Realistic adc + irq -> 15 us = 66 KHz
//   66 KHz / 1000 samples/rev => 4000 RPM max
//   => impossible
// GPIO + IRQ:
//   gpio + irq -> 5 us = 200 KHz = 1200 k RPM

#define SAMPLE_HIST_BINS    4096  // Map 0-4096 (resolution 12 bits hardcoded) (0-5 V)
#define REV_HIST_BINS       4096  // Map 0-4096 ms (1-100 k RPM = 0.6 - 60 ms)
#define IRQ_HIST_BINS       4096  // Map 0-4096 us

#define PRINT_CYCLE_SEC     1.000

// ----------------------- Globals -----------------------

nod_mutex_t timer_mutex = {0};

// written by IRQ, needs volatile
volatile uint32_t sample_hist_irq[SAMPLE_HIST_BINS] = {0}; // To determine rev threshold
volatile uint32_t rev_duration_hist_irq[REV_HIST_BINS] = {0}; // Make sure rev count is clean and reliable
volatile uint32_t irq_duration_hist_irq[IRQ_HIST_BINS] = {0}; // Measure adc read duration

// only read by IRQ
uint32_t threshold_index_irq = 0;
uint32_t rev_count_irq = 0;

// ----------------------- Utilities -----------------------

uint32_t nod_rpm_to_pulse_us(uint32_t rpm)
{
    const uint32_t rpm_clamped = (rpm < REV_MIN_RPM) ? REV_MIN_RPM : (rpm > REV_MAX_RPM) ? REV_MAX_RPM : rpm;
    return PWM_MIN_US + (rpm_clamped - REV_MIN_RPM) * (PWM_MAX_US - PWM_MIN_US) / (REV_MAX_RPM - REV_MIN_RPM);
}

void nod_esc_arm(void)
{
    nod_printf("Arming ESC...\n");

    // Option A: standard low-high-low
    nod_pwm_write_us(PWM_PIN, PWM_FREQ_HZ, PWM_MIN_US);
    nod_time_sleep_sec(2.000);
    nod_pwm_write_us(PWM_PIN, PWM_FREQ_HZ, PWM_MAX_US);
    nod_time_sleep_sec(2.000);
    nod_pwm_write_us(PWM_PIN, PWM_FREQ_HZ, PWM_MIN_US);
    nod_time_sleep_sec(2.000);
    nod_printf("ESC armed (standard sequence)\n");

    /*
    // Option B: simple safe-low only
    nod_pwm_write_us(PWM_PIN, PWM_FREQ_HZ, PWM_MIN_US);
    nod_time_sleep_sec(3.000);
    nod_printf("ESC armed (safe-low only)\n");
    */
}

// ----------------------- ISR -----------------------

void NOD_IRAM_ATTR nod_timer_irq(void)
{
    static bool last_sample_above_threshold = false;
    static uint32_t rev_start_us_last = 0;

    nod_mutex_lock(&timer_mutex);

    const uint32_t start_us = nod_time_get_us();

    // read sample
    const uint32_t adc_raw = nod_adc1_read(ADC_PIN);
    const uint32_t sample_index = adc_raw;
    if (0 <= sample_index && sample_index < SAMPLE_HIST_BINS)
    {
        sample_hist_irq[sample_index] ++;
    }

    // crossing detection with basic debounce
    if (sample_index >= threshold_index_irq)
    {
        if (last_sample_above_threshold == false)
        {
            last_sample_above_threshold = true;

            rev_count_irq++;

            // compute rev duration
            const uint32_t rev_duration_us = start_us - rev_start_us_last;
            const uint32_t rev_duration_index = rev_duration_us / 1000;
            if (0 <= rev_duration_index && rev_duration_index < REV_HIST_BINS)
            {
                rev_duration_hist_irq[rev_duration_index] ++;
            }
            rev_start_us_last = start_us;
        }
    }
    else
    {
        last_sample_above_threshold = false;
    }

    // ISR timing profiling
    const uint32_t dur_us = nod_time_get_us() - start_us;
    const uint32_t dur_index = dur_us;
    if (0 <= dur_index && dur_index < IRQ_HIST_BINS)
    {
        irq_duration_hist_irq[dur_index]++;
    }

    nod_mutex_unlock(&timer_mutex);
}

// ----------------------- Main -----------------------

uint32_t mapper(uint32_t param)
{
    return param;
}

int main(void)
{
    nod_time_init();

    nod_stdout_init(SERIAL_BAUD);

    nod_time_sleep_sec(1.000);
    nod_printf("\n");
    nod_printf("+---------------------------------+\n");
    nod_printf("| Project F - Foucault experiment |\n");
    nod_printf("+---------------------------------+\n");
    nod_printf("\n");
    nod_time_sleep_sec(1.000);

    nod_pwm_init(PWM_PIN, PWM_FREQ_HZ);
    nod_pwm_write_us(PWM_PIN, PWM_FREQ_HZ, PWM_MIN_US);

    nod_adc1_init(ADC_PIN);

    nod_mutex_init(&timer_mutex);

    // start motor control and rev counting

    threshold_index_irq = SAMPLE_HIST_BINS/2;  // init to average
    uint32_t rpm_command = 1200;

    nod_esc_arm();

    nod_pwm_write_us(PWM_PIN, PWM_FREQ_HZ, nod_rpm_to_pulse_us(rpm_command));

    nod_timer_t timer;
    nod_timer_init(
        &timer,
        TIMER_FREQUENCY,
        /* callback_period_sec */ 1 / (SAMPLES_PER_REV * (rpm_command / 60)) /* => 1 K RPM * 1000 sample / revolution = 60 us */,
        &nod_timer_irq
    );

    double last_print_sec = nod_time_get_sec();
    while (true)
    {
        //
        // Read serial input
        //

        char buf[128] = "";

        while (nod_stdin_peek())
        {
            const char c = nod_stdin_read();
            if (c == '\r')
            {
                continue;
            }

            if (c == '\n')
            {
                rpm_command = atoi(buf);
                buf[0] = '\0';

                // Update PWM
                if (rpm_command / 60 == 0)
                    rpm_command = 60;
                nod_printf("cmd_rpm = %d RPM ; rev dur = %d us\n", rpm_command, 1000 * 1000 / (rpm_command / 60));
                nod_pwm_write_us(PWM_PIN, PWM_FREQ_HZ, nod_rpm_to_pulse_us(rpm_command));
            }
            else
            {
                const uint32_t len = strlen(buf);
                buf[len] = c;
                buf[len + 1] = '\0';
            }
        }

        //
        // 1 Hz status print
        //

        const double now_sec = nod_time_get_sec();

        if (now_sec - last_print_sec >= PRINT_CYCLE_SEC)
        {
            ///
            // copy IRQ-owned data to local working copy, and reset IRQ copy
            ///

            nod_mutex_lock(&timer_mutex);

            static uint32_t sample_hist_thread[SAMPLE_HIST_BINS] = {0}; // To determine rev threshold
            static uint32_t rev_duration_hist_thread[REV_HIST_BINS] = {0}; // Make sure rev count is clean and reliable
            static uint32_t irq_duration_hist_thread[IRQ_HIST_BINS] = {0}; // Measure adc read duration

            uint32_t rev_count_thread = rev_count_irq;

            memcpy(sample_hist_thread, (uint32_t*)sample_hist_irq, sizeof(sample_hist_irq));
            memcpy(rev_duration_hist_thread, (uint32_t*)rev_duration_hist_irq, sizeof(rev_duration_hist_irq));
            memcpy(irq_duration_hist_thread, (uint32_t*)irq_duration_hist_irq, sizeof(irq_duration_hist_irq));

            rev_count_irq = 0;

            memset((uint32_t*)sample_hist_irq, 0, sizeof(sample_hist_irq));
            memset((uint32_t*)rev_duration_hist_irq, 0, sizeof(rev_duration_hist_irq));
            memset((uint32_t*)irq_duration_hist_irq, 0, sizeof(irq_duration_hist_irq));

            nod_mutex_unlock(&timer_mutex);

            //
            // Fancy weighted average => seems ok?, but complex
            //

            uint32_t sum_below_threshold = threshold_index_irq;
            uint32_t sample_total_count_below_threshold = 1; //  avoid div by zero
            for (uint32_t i = 0; i < threshold_index_irq; i++)
            {
                sum_below_threshold += i * sample_hist_thread[i];
                sample_total_count_below_threshold += sample_hist_thread[i];
            }
            const uint32_t average_below = sum_below_threshold / sample_total_count_below_threshold;

            uint32_t sum_above_threshold = threshold_index_irq;
            uint32_t sample_total_count_above_threshold = 1; //  avoid div by zero
            for (uint32_t i = threshold_index_irq; i < SAMPLE_HIST_BINS; i++)
            {
                sum_above_threshold += i * sample_hist_thread[i];
                sample_total_count_above_threshold += sample_hist_thread[i];
            }
            const uint32_t average_above = sum_above_threshold / sample_total_count_above_threshold;

            const float percent_below = 1.0 * sample_total_count_below_threshold / (sample_total_count_below_threshold + sample_total_count_above_threshold);

            const uint32_t average = (average_below + average_above) / 2;

            threshold_index_irq = average;

            //
            // print
            //

            nod_printf("\n");

            if (rpm_command / 60 == 0)
                rpm_command = 60;

            if (rev_count_thread == 0)
                rev_count_thread = 1;

            nod_printf("cmd_rpm  = %7.3f k RPM ; cmd_rev_dur  = %7.3f ms\n", rpm_command / 1000.0, 1000.0 / (rpm_command / 60.0));
            nod_printf("meas_rpm = %7.3f k RPM ; meas_rev_dur = %7.3f ms\n", rev_count_thread * 60.0 / 1000.0, 1000.0 / rev_count_thread);
            nod_printf(
                "threshold_index_irq = %4d [0-%d] = %5.3f V // samples: %.1f%% < threshold < %.1f%%\n",
                threshold_index_irq, SAMPLE_HIST_BINS, threshold_index_irq * 5.0 / SAMPLE_HIST_BINS, 100 * percent_below, 100 - 100 * percent_below
            );

            // print sample stats

            struct nod_stats_t stats_sample_hist = {0};
            nod_stats_init(&stats_sample_hist, sample_hist_thread, SAMPLE_HIST_BINS, mapper, mapper);
            nod_stats_print(&stats_sample_hist, "sample_hist (adc raw: 0-4096)");

            // print rev duration stats

            struct nod_stats_t stats_rev_duration = {0};
            nod_stats_init(&stats_rev_duration, rev_duration_hist_thread, REV_HIST_BINS, mapper, mapper);
            nod_stats_print(&stats_rev_duration, "rev_duration (ms) (1 k RPM = 60 ms ; 100 k RPM = 0.6 ms)");

            // print IRQ duration stats

            struct nod_stats_t stats_irq_duration = {0};
            nod_stats_init(&stats_irq_duration, irq_duration_hist_thread, IRQ_HIST_BINS, mapper, mapper);
            nod_stats_print(&stats_irq_duration, "irq_duration (us) (100 k RPM * 1000 samples/rev = 0.7 us ; Realistic ESP32 = 15 us");

            nod_printf("\n");

            last_print_sec += 1.000;
        }

        nod_time_sleep_sec(0.001);
    }

    return 0;
}
