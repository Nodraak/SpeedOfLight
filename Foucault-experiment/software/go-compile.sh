set -e
set -x

gcc -O0 -g -lpthread -I hal/linux/ -x c project-f.ino nod_stats.ino hal/nod_hal_common.ino hal/linux/nod_hal_linux.c

# rm -r ~/.arduino-build/

