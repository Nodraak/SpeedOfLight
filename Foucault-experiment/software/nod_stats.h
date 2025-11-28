#pragma once

#include <stdint.h>

struct nod_stats_t {
    uint32_t *buffer;
    uint32_t buffer_size;
    uint32_t (*sample2index)(uint32_t param);
    uint32_t (*index2sample)(uint32_t param);
};

void nod_stats_init(
    struct nod_stats_t *stat,
    uint32_t *buf,
    uint32_t buffer_size,
    uint32_t (*sample2index)(uint32_t param),
    uint32_t (*index2sample)(uint32_t param)
);

// void nod_stats_add_sample(nod_stats_t *stats, uint32_t sample);

void nod_stats_print(struct nod_stats_t *stats, const char *title);
