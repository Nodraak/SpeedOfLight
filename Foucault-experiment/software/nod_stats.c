#include "nod_stats.h"

#include "nod_hal.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

void nod_stats_init(
    nod_stats_t *stat,
    uint32_t *buf,
    uint32_t buffer_size,
    uint32_t (*sample2index)(uint32_t param),
    uint32_t (*index2sample)(uint32_t param)
)
{
    stat->buffer = buf;
    stat->buffer_size = buffer_size;
    stat->sample2index = sample2index;
    stat->index2sample = index2sample;

    // memset(stat->buffer, 0, sizeof(stat->buffer[0]) * stat->buffer_size);
}

// void nod_stats_add_sample(nod_stats_t *stats, uint32_t sample)
// {
//     uint32_t index = stats->sample2index(sample);
//     if (index < 0)
//     {
//         index = 0;
//     }
//     if (stats->buffer_size <= index)
//     {
//         index = stats->buffer_size - 1;
//     }
//
//     stats->buffer[index] += 1;
// }

void nod_stats_print_stats(nod_stats_t *stats, const char *title)
{
    uint32_t min = 0, max = 0, median = 0, deciles[10] = {0};
    uint32_t sample_counter = 0;

    uint32_t sample_total_count = 0;
    for (uint32_t i = 0; i < stats->buffer_size; i++)
    {
        sample_total_count += stats->buffer[i];
    }

    if (sample_total_count)
    {
        // min
        for (uint32_t i = 0; i < stats->buffer_size; i++)
        {
            if (stats->buffer[i] != 0)
            {
                min = stats->index2sample(i);
                break;
            }
        }

        // max
        for (uint32_t i = stats->buffer_size - 1; i > 0; i--)
        {
            if (stats->buffer[i] != 0)
            {
                max = stats->index2sample(i);
                break;
            }
        }

        // median
        sample_counter = 0;
        for (uint32_t i = 0; i < stats->buffer_size; i++)
        {
            sample_counter += stats->buffer[i];

            if (sample_counter >= sample_total_count/2)
            {
                median = stats->index2sample(i);
                break;
            }
        }

        // deciles
        sample_counter = 0;
        uint32_t current_decile = 0;
        for (uint32_t i = 0; i < stats->buffer_size; i++)
        {
            sample_counter += stats->buffer[i];

            while (sample_counter >= sample_total_count * (current_decile + 1) / 10)
            {
                deciles[current_decile] = stats->index2sample(i);
                current_decile++;
            }
        }
    }

    // print
    // 160 char = 15 ms
    nod_printf("/--- %s\n", title);
    nod_printf("| sample_total_count=%6d\n", sample_total_count);
    nod_printf("| min=%6d | med=%6d | max=%6d\n", min, median, max);
    nod_printf("| ds=");
    for (uint32_t i = 0; i < 10; i++)
    {
        nod_printf(" %6d", deciles[i]);
    }
    nod_printf("\n");
    nod_printf("\\---\n");
}
