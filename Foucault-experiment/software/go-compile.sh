set -e
set -x

gcc -I hal/linux/ project-f.c hal/linux/nod_hal_linux.c
# -lpthread
