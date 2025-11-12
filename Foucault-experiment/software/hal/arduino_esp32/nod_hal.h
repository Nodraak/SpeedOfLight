#pragma once

#include "../nod_hal_common.h"

#include <Arduino.h>
#include "esp_timer.h"

typedef struct nod_timer_t {
    hw_timer_t *data;
} nod_timer_t;

typedef struct nod_mutex_t {
    portMUX_TYPE data;
} nod_mutex_t;
