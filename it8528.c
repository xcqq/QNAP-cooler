#include <stdlib.h>
#include <stdio.h>
#include <sys/io.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#include "it8528.h"


#define IT8528_CHECK_RETRIES 400
#define IT8528_INPUT_BUFFER_FULL 2
#define IT8528_OUTPUT_BUFFER_FULL 1


static int it8528_check_bit(uint8_t port, uint8_t bit_value) {
    
    int retries = IT8528_CHECK_RETRIES;
    int value = 0;

    do { 
        value = inb(port);
        struct timespec ts = { .tv_sec= 0, .tv_nsec = 0x32 * 1000};
        nanosleep(&ts, NULL);

        if ((value & bit_value) == 0) {
           return 0;
        }
    }
    while (retries--);

    return -ETIMEDOUT;
}


static int it8528_clear_buffer(uint8_t port) {
    int retries = 5000;
    int value;

    do {
        value = inb(0x6C);
        struct timespec ts = { .tv_sec= 0, .tv_nsec = 0x32 * 1000};
        nanosleep(&ts, NULL);

        if ((value & 1) != 0) {
           return 0;
        }
    }
    while (retries--);

    return -ETIMEDOUT;
}

static int it8528_send_commands(uint8_t command0, uint8_t command1) {
    int ret;

    ret = it8528_check_bit(0x6C, IT8528_OUTPUT_BUFFER_FULL);
    if (ret != 0) {
        goto err;
    }
    inb(0x68);

    ret = it8528_check_bit(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret != 0) {
        goto err;
    }
    outb(0x88, 0x6C);

    ret = it8528_check_bit(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret != 0) {
        goto err;
    }
    outb(command0, 0x68);

    ret = it8528_check_bit(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret != 0) {
        goto err;
    }
    outb(command1, 0x68);

    ret = it8528_check_bit(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret != 0) {
        goto err;
    }

    return 0;
err:
    fprintf(stderr, "%d:%s: it8528 is not ready\n", __LINE__, __func__);
    return ret;
 }


static int it8528_get_double(uint8_t command0, uint8_t command1, double *value) {
    int ret;

    if ((inb(0x6C) & 1) != 0) {
        it8528_clear_buffer(0x68);
        inb(0x68);
    }

    ret = it8528_send_commands(command0, command1);
    if (ret != 0) {
        return ret;
    }
    it8528_clear_buffer(0x68);
    *value = inb(0x68);

    return 0;
}


static int it8528_get_byte(uint8_t command0, uint8_t command1, uint8_t *value) {
    int ret;

    if ((inb(0x6C) & 1) != 0) {
        it8528_clear_buffer(0x68);
        inb(0x68);
    }

    ret = it8528_send_commands(command0, command1);
    if (ret != 0) {
        fprintf(stderr, "%d:%s: get byte fail\n", __LINE__, __func__);
        return ret;
    }
    it8528_clear_buffer(0x68);
    *value = inb(0x68);

    return 0;
}


static int it8528_set_byte(uint8_t command0, uint8_t command1, uint8_t value) {
    int8_t ret;

    ret = it8528_check_bit(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret != 0) {
        goto err;
    }
    outb(0x88, 0x6C);

    ret = it8528_check_bit(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret != 0) {
        goto err;
    }
    outb(command0 | 0x80, 0x68);

    ret = it8528_check_bit(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret != 0) {
        goto err;
    }
    outb(command1, 0x68);

    ret = it8528_check_bit(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret != 0) {
        goto err;
    }
    outb(value, 0x68);
     
    return 0;
err:
    fprintf(stderr, "%d:%s: it8528 set byte fail\n", __LINE__, __func__);
    return ret;
}



int it8528_get_fan_status(unsigned int fan_id, uint8_t* status_value) {
    int8_t ret;

    if (fan_id > 5) {
        fprintf(stderr, "%d:%s: fan ID too big\n", __LINE__, __func__);
        return -1;
    }

    if(ioperm(0x6c, 1, 1) != 0) {
        fprintf(stderr, "%d:%s: ioperm(0x6c) failed\n", __LINE__, __func__);
        return -1;
    }

    if(ioperm(0x68, 1, 1) != 0) {
        fprintf(stderr, "%d:%s: ioperm(0x68) failed\n", __LINE__, __func__);
        return -1;
    }

    ret = it8528_get_byte(2, 0x42, status_value);
    if (ret != 0) {
        fprintf(stderr, "%d:%s: get 0x42 failed\n", __LINE__, __func__);
        return ret;
    }
    *status_value = (*status_value & 1) ^ 1;

    return 0;
}


int it8528_get_fan_pwm(unsigned int fan_id, uint8_t* pwm_value) {

    uint8_t tmp_pwm_value = 0;
    uint8_t command = 0;

    if(ioperm(0x6c, 1, 1) != 0) {
        fprintf(stderr, "it8528_get_fan_pwm: ioperm(0x6c) failed!\n");
        return -1;
    }

    if(ioperm(0x68, 1, 1) != 0) {
        fprintf(stderr, "it8528_get_fan_pwm: ioperm(0x68) failed!\n");
        return -1;
    }

    if (fan_id < 0) {
        fprintf(stderr, "it8528_get_fan_pwm: invalid fan ID!\n");
        return -1;
    }
    if (fan_id < 5) {
        command = 0x2e;
    }
    else {
        command = 0x4b;
    }

    int8_t ret;
   
    ret = it8528_get_byte(2, command, &tmp_pwm_value);
    if (ret != 0) {
        fprintf(stderr, "it8528_get_fan_pwm: it8528_get_byte() failed!\n");
        return ret;
    }

    *pwm_value = (tmp_pwm_value * 0x100 - tmp_pwm_value) / 100;

    return 0;
}


int it8528_get_fan_speed(unsigned int fan_id, uint32_t* speed_value) {

    uint8_t byte0;
    uint8_t byte1;

    if(ioperm(0x6c, 1, 1) != 0) {
        fprintf(stderr, "it8528_get_fan_speed: ioperm(0x6c) failed!\n");
        return -1;
    }

    if(ioperm(0x68, 1, 1) != 0) {
        fprintf(stderr, "it8528_get_fan_speed: ioperm(0x68) failed!\n");
        return -1;
    }

    switch(fan_id) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
            byte0 = (fan_id * 2) + 0x24;
            byte1 = (fan_id * 2) + 0x25;
            break;
        case 6:
        case 7:
            byte0 = (fan_id * 2) + 0x14;
            byte1 = (fan_id * 2) + 0x15;
            break;
        case 10:
            byte1 = 0x5a;
            byte0 = 0x5b;
            break;
        case 11:
            byte1 = 0x5d;
            byte0 = 0x5e;
            break;
        default:
            fprintf(stderr, "it8528_get_fan_speed: invalid fan ID!\n");
            return -1;
    }

    int8_t ret;
    uint8_t tmp_value = 0;

    ret = it8528_get_byte(6, byte0, &tmp_value);
    if (ret != 0) {
        fprintf(stderr, "it8528_get_fan_speed: it8528_get_byte() #1 failed!\n");
        return ret;
    }
    *speed_value = ((uint32_t) tmp_value) << 8;

    ret = it8528_get_byte(6, byte1, &tmp_value);
    if (ret != 0) {
        fprintf(stderr, "it8528_get_fan_speed: it8528_get_byte() #2 failed!\n");
        return ret;
    }
    *speed_value += tmp_value;

    return 0;
}


int it8528_get_temperature(unsigned int sensor_id, double* temperature_value) {

    uint8_t command = 0;


    if(ioperm(0x6c, 1, 1) != 0) {
        return -1;
    }

    if(ioperm(0x68, 1, 1) != 0) {
        return -1;
    }

    command = sensor_id;
 
    switch(sensor_id) {
         case 0:
         case 1:
             break;
         case 5:
         case 6:
         case 7:
             command -= 3;
             break;
         case 10:
             command = 0x59;
             break;
         case 11:
             command = 0x5C;
             break;
         case 0x0F:
         case 0x10:
         case 0x11:
         case 0x12:
         case 0x13:
         case 0x14:
         case 0x15:
         case 0x16:
         case 0x17:
         case 0x18:
         case 0x19:
         case 0x1A:
         case 0x1B:
         case 0x1C:
         case 0x1D:
         case 0x1E:
         case 0x1F:
         case 0x20:
         case 0x21:
         case 0x22:
         case 0x23:
         case 0x24:
         case 0x25:
         case 0x26:
             command = command - 9;
             break;
         default:
             command = 0xD6;
             break;
    }

    int8_t ret;

    ret = it8528_get_double(6, command, temperature_value);
    if (ret != 0) {
        return ret;
    }

    return 0;
}

int it8528_set_fan_speed(unsigned int fan_id, uint8_t fan_speed) {

    uint8_t command0, command1;
    int ret;

    if (fan_id < 0) {
        return -1;
    }

    if(ioperm(0x6c, 1, 1) != 0) {
        return -1;
    }

    if(ioperm(0x68, 1, 1) != 0) {
        return -1;
    }

    if (fan_id < 5) {
        command0 = 0x20;
        command1 = 0x2e;
    }
    else {
        command0 = 0x23;
        command1 = 0x4b;
    }

    ret = it8528_set_byte(2, command0, 0x10);
    if (ret != 0) {
        return ret;
    }

    ret = it8528_set_byte(2, command1, fan_speed);
    if (ret != 0) {
        return ret;
    }

    return 0;
}
