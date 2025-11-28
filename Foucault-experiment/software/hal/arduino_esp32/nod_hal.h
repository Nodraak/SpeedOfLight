#pragma once

#include "nod_hal_common.h"

// TODO
#define NOD_IRAM_ATTR ARDUINO_ISR_ATTR
// #define NOD_IRAM_ATTR IRAM_ATTR

typedef struct nod_timer_t {
    hw_timer_t *data;
} nod_timer_t;

typedef struct nod_mutex_t {
    portMUX_TYPE data;
} nod_mutex_t;
