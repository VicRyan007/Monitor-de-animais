#include "dht.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

static const char *TAG = "DHT";

static esp_err_t dht_await_pin(gpio_num_t pin, uint32_t timeout, int expected_level, uint32_t *duration) {
    for (uint32_t i = 0; i < timeout; i += 1) {
        if (gpio_get_level(pin) == expected_level) {
            if (duration) *duration = i;
            return ESP_OK;
        }
        ets_delay_us(1);
    }
    return ESP_ERR_TIMEOUT;
}

esp_err_t dht_read(gpio_num_t pin, struct dht_reading *reading) {
    uint8_t data[5] = {0, 0, 0, 0, 0};

    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    ets_delay_us(20000);
    gpio_set_level(pin, 1);
    ets_delay_us(40);
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    if (dht_await_pin(pin, 40, 0, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "Pin never went low");
        return ESP_FAIL;
    }
    if (dht_await_pin(pin, 80, 1, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "Pin never went high");
        return ESP_FAIL;
    }
    if (dht_await_pin(pin, 80, 0, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "Pin never went low after 80us");
        return ESP_FAIL;
    }

    for (int i = 0; i < 40; i++) {
        uint32_t low_duration;
        uint32_t high_duration;

        if (dht_await_pin(pin, 50, 1, &low_duration) != ESP_OK) {
            ESP_LOGE(TAG, "Low signal duration too long");
            return ESP_FAIL;
        }
        if (dht_await_pin(pin, 70, 0, &high_duration) != ESP_OK) {
            ESP_LOGE(TAG, "High signal duration too long");
            return ESP_FAIL;
        }

        data[i / 8] <<= 1;
        if (high_duration > low_duration) {
            data[i / 8] |= 1;
        }
    }

    // LINHA CORRIGIDA COM PARÊNTESES EXTRAS
    if (((data[0] + data[1] + data[2] + data[3]) & 0xFF) != data[4]) {
        ESP_LOGE(TAG, "Checksum failed");
        return ESP_FAIL;
    }

    reading->humidity = data[0] + (float)data[1] / 10;
    reading->temperature = data[2] + (float)data[3] / 10;

    return ESP_OK;
}