#include "driver/gpio.h"
#pragma once
int readDHT11(gpio_num_t pin, double* temp, double* humid);
