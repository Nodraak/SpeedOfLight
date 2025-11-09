// #include <Arduino.h>
// #include "driver/adc.h"
// #include "esp_adc_cal.h"
#include "stdint.h"

// ----------------------- User config -----------------------

#define LEDC_CHANNEL    0
#define LEDC_TIMER      0
#define SERIAL_BAUD     115200

#define PWM_PIN         32
#define ADC_PIN         33 // ADC1 channel 5

#define ADC_CHANNEL     ADC1_CHANNEL_5 // GPIO33
#define ADC_ATTEN       ADC_ATTEN_DB_11
#define ADC_WIDTH_BITS  ADC_WIDTH_BIT_12

#define PWM_FREQ_HZ     50
#define PWM_MIN_US      1000
#define PWM_MAX_US      2000

// TODO
#define SAMPLE_US       62   // ~16 kHz = 1 000 000 RPM
// 100 k RPM, 100 samples

#define VAL_HIST_BINS   256
#define IRQ_HIST_BINS   1024
#define REV_HIST_BINS   1024

#define PRINT_CYCLE_MS  1000

// ----------------------- Globals -----------------------

portMUX_TYPE isrMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t sample_count = 0, crossing_count = 0;
volatile uint32_t val_hist[VAL_HIST_BINS] = {0};
volatile uint32_t irq_timing_hist[IRQ_HIST_BINS] = {0};
volatile uint32_t rev_timing_hist[REV_HIST_BINS] = {0};

uint32_t threshold_bin = 0;

// ----------------------- Utilities -----------------------

uint32_t nod_pulseUS_to_duty(uint32_t pulse_us, uint32_t res_bits)
{
    return (uint32_t)(
        (
            (uint64_t)pulse_us * ((1 << res_bits) - 1)
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
    Serial.println("Arming ESC...");

    // Option A: standard low-high-low
    ledcWrite(LEDC_CHANNEL, nod_pulseUS_to_duty(PWM_MIN_US, 16));
    delay(2000);
    ledcWrite(LEDC_CHANNEL, nod_pulseUS_to_duty(PWM_MAX_US, 16));
    delay(2000);
    ledcWrite(LEDC_CHANNEL, nod_pulseUS_to_duty(PWM_MIN_US, 16));
    delay(2000);
    Serial.println("ESC armed (standard sequence)");

    /*
    // Option B: simple safe-low only
    ledcWrite(LEDC_CHANNEL, nod_pulseUS_to_duty(PWM_MIN_US,16));
    delay(3000);
    Serial.println("ESC armed (safe-low only)");
    */
}

// ----------------------- ISR -----------------------

void IRAM_ATTR nod_timer_irq(void)
{
    static bool currently_above = false;

    const uint32_t start = esp_timer_get_time();

    const int raw = adc1_get_raw(ADC_CHANNEL);

    const uint32_t bin = (uint32_t)((raw*HIST_BINS)/4096);

    portENTER_CRITICAL_ISR(&isrMux);

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
    const uint32_t dur = esp_timer_get_time() - start;
    if (dur >= 1 && dur <= 1000)
    {
        irq_timing_hist[dur-1]++;
    }

    portEXIT_CRITICAL_ISR(&isrMux);
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

// ----------------------- Setup -----------------------

void setup(void)
{
    Serial.begin(SERIAL_BAUD);
    delay(200 /* ms */);
    Serial.println("\nESP32 ESC+RPM+Profiler");

    adc1_config_width(ADC_WIDTH_BITS);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

    ledcSetup(LEDC_CHANNEL, PWM_FREQ_HZ, 16);
    ledcAttachPin(PWM_PIN, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, nod_pulseUS_to_duty(PWM_MIN_US, 16));

    for (int i = 0; i < VAL_HIST_BINS; i++)
    {
        val_hist[i] = 0;
    }

    // void timerAlarm(hw_timer_t * timer, uint64_t alarm_value, bool autoreload, uint64_t reload_count);
    // timer = timerBegin(1000000);
    // timerAttachInterrupt(timer, &onTimer);
    // timerAlarm(timer, 1000000, true, 0);

    hw_timer_t *timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &nod_timer_irq, true);
    timerAlarmWrite(timer, SAMPLE_US, true);
    timerAlarmEnable(timer);

    nod_armESC();
}

// ----------------------- Main loop -----------------------

void loop(void)
{
    static uint32_t last_print_ms = millis();

    // Read serial input

    uint32_t rpm_command = 1000;
    static String buf = "";

    while (Serial.available())
    {
        const char c = Serial.read();
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
                Serial.printf("[CMD] RPM = %u\n", rpm_command);

                // Update PWM
                ledcWrite(LEDC_CHANNEL, nod_pulseUS_to_duty(nod_rpm_to_pulse_us(rpm_command), 16));
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

    const uint32_t now = millis();

    if (now - last_print_ms >= PRINT_CYCLE_MS)
    {
        uint32_t local_hist[VAL_HIST_BINS], local_count, local_cross;

        portENTER_CRITICAL(&isrMux);

        for (int i = 0; i < VAL_HIST_BINS; i++)
        {
            local_hist[i] = val_hist[i];
            val_hist[i] = 0;
        }

        local_count = sample_count;
        sample_count = 0;
        local_cross = crossing_count;
        crossing_count = 0;

        portEXIT_CRITICAL(&isrMux);

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

        portENTER_CRITICAL(&isrMux);

        for (int i = 0;i < IRQ_HIST_BINS; i++)
        {
            timing_snapshot[i] = irq_timing_hist[i];
            irq_timing_hist[i] = 0;
        }

        portEXIT_CRITICAL(&isrMux);

        nod_deciles(timing_snapshot, IRQ_HIST_BINS, local_count, timing_dec);

        // Rotation duration histogram

        uint32_t rot_snapshot[REV_HIST_BINS];
        uint8_t rot_dec[10];

        portENTER_CRITICAL(&isrMux);

        for (int i = 0; i < REV_HIST_BINS; i++)
        {
            rot_snapshot[i] = rev_timing_hist[i];
            rev_timing_hist[i] = 0;
        }

        portEXIT_CRITICAL(&isrMux);

        // Print

        Serial.printf(
            "[STAT] cmdRPM = %u measuredRPM = %u samples = %u threshold = %u ISRdec = ",
            rpm_command, measured_rpm, local_count, threshold_bin
        );

        for (int i = 0; i < 10; i++)
        {
            Serial.printf("%u%s", timing_dec[i], (i < 9) ? "," : "");
        }

        Serial.println();

        last_print_ms += 1000;
    }

    delay(1 /* ms */);
}
