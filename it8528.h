#ifndef IT8528_H_
#define IT8528_H_
int it8528_get_fan_pwm(unsigned int, uint8_t*);
int it8528_get_fan_speed(unsigned int, uint32_t*);
int it8528_get_fan_status(unsigned int, uint8_t*);
int it8528_get_temperature(unsigned int, double*);
int it8528_set_fan_speed(unsigned int, uint8_t);
#endif