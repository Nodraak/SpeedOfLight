#pragma once

#include "../nod_hal_common.h"

#include <pthread.h>

#define NOD_ADC_STUB_FILEPATH "hal/linux/adc-data-1.txt"
// #define NOD_ADC_STUB_FILEPATH "hal/linux/adc-data-10.txt"

#define NOD_IRAM_ATTR

typedef struct nod_timer_t {
    pthread_t thread;
    uint64_t delay_us;
    void (*userFunc)(void);
} nod_timer_t;

typedef struct nod_mutex_t {
    uint32_t dummy;
} nod_mutex_t;
