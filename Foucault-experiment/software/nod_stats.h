#pragma once

#include <stdint.h>

typedef struct {
    uint32_t *buffer;
    uint32_t buffer_size;
    uint32_t (*sample2index)(uint32_t param);
    uint32_t (*index2sample)(uint32_t param);
} nod_stats_t;

void nod_stats_init(
    nod_stats_t *stat,
    uint32_t *buf,
    uint32_t buffer_size,
    uint32_t (*sample2index)(uint32_t param),
    uint32_t (*index2sample)(uint32_t param)
);

// void nod_stats_add_sample(nod_stats_t *stats, uint32_t sample);

void nod_stats_compute_deciles(nod_stats_t *stats, uint32_t deciles[10]);

void nod_stats_print_stats(nod_stats_t *stats, const char *title);
