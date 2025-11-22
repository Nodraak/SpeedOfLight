set -e
set -x

gcc -O0 -g -lpthread -I hal/linux/ project-f.c nod_stats.c hal/nod_hal_common.c hal/linux/nod_hal_linux.c
