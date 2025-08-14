#ifndef __DHT_H__
#define __DHT_H__

#include <driver/gpio.h>

enum dht_sensor_type {
    DHT_TYPE_DHT11,
    DHT_TYPE_AM2301,
};

struct dht_reading {
    float temperature;
    float humidity;
};

esp_err_t dht_read(gpio_num_t pin, struct dht_reading *reading);

#endif
