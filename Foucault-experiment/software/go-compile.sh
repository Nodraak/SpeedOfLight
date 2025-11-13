set -e
set -x

gcc -I hal/linux/ project-f.c nod_stats.c hal/linux/nod_hal_linux.c
# -lpthread
