#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "it8528.h"

#define DEFALT_FAN_ID 0

void usage(void) {
    printf("Usage: fan_controller [-m pwm] [-a fan_config]\n    \
Options:\n\
-m, -m pwm\n\
    Manually set fan speed.\n\
-a, -a high,mid,low_temp high,mid,low_pwm\n\
    Automatically set fan speed, the fan speed may\n\
    set to target pwm when over the target temp.\n\
");

    exit(EXIT_FAILURE);
}

int set_fan_pwm(int pwm) {
    int ret;
    ret = it8528_set_fan_speed(DEFALT_FAN_ID, pwm);
    return ret;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
    }
    if(strcmp(argv[1], "-h") == 0) {
        usage();
    } else if(strcmp(argv[1], "-m") == 0) {
        set_fan_pwm(atoi(argv[2]));
    } else {
        printf("Unkown argument:%s\n", argv[1]);
        usage();
    }

    exit(EXIT_SUCCESS);
}