#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <math.h>

// Biblioteca do sensor DHT (criada manualmente na pasta components)
#include "dht.h"

// ============================================================================
// CONFIGURAÇÕES - MANTIDAS DO SEU PROJETO ORIGINAL
// ============================================================================
#define WIFI_SSID      "Seu wifi"
#define WIFI_PASS      "Sua senha"
#define MQTT_BROKER_IP "192.168.3.2" // Use apenas o IP
#define DEVICE_ID      "sensor_animais_01"

// --- Configurações Técnicas ---
#define MQTT_TOPIC     "esp32/dht11/data"
#define DHT11_GPIO_PIN (gpio_num_t)4
#define PUBLISH_INTERVAL_MS 10000

// ============================================================================
// DEFINIÇÕES E VARIÁVEIS GLOBAIS
// ============================================================================
static const char *TAG = "MONITOR_ANIMAIS";
static EventGroupHandle_t s_event_group;
static esp_mqtt_client_handle_t mqtt_client = NULL;

#define WIFI_CONNECTED_BIT BIT0
#define MQTT_CONNECTED_BIT BIT1

// ============================================================================
// FUNÇÕES DE MELHORIA DE DADOS
// ============================================================================
static float calculate_heat_index(float temperature, float humidity) {
    if (temperature < 26.7) return temperature;
    float T = temperature;
    float H = humidity;
    return -8.784695 + 1.61139411 * T + 2.338549 * H - 0.14611605 * T * H - 0.01230809 * T * T - 0.016424828 * H * H + 0.002211732 * T * T * H + 0.00072546 * T * H * H - 0.000003582 * T * T * H * H;
}

static const char* get_comfort_status(float temperature) {
    if (temperature < 15.0) return "Frio";
    if (temperature >= 15.0 && temperature <= 26.0) return "Confortavel";
    if (temperature > 26.0 && temperature <= 32.0) return "Quente";
    return "Perigoso";
}

// ============================================================================
// FUNÇÕES DE WI-FI E MQTT
// ============================================================================
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Conexão Wi-Fi perdida. Tentando reconectar...");
        xEventGroupClearBits(s_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Conectado ao Wi-Fi!");
        xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
    }
}

static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conectado ao broker MQTT");
            xEventGroupSetBits(s_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Desconectado do broker MQTT");
            xEventGroupClearBits(s_event_group, MQTT_CONNECTED_BIT);
            break;
        default:
            break;
    }
}

static void setup_connections(void) {
    s_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS, }, };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    xEventGroupWaitBits(s_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    esp_mqtt_client_config_t mqtt_cfg = { .broker.address.uri = "mqtt://" MQTT_BROKER_IP, };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
}

// ============================================================================
// TAREFA PRINCIPAL DE LEITURA E PUBLICAÇÃO
// ============================================================================
static void sensor_publish_task(void *pvParameters) {
    char json_payload[256];
    struct dht_reading reading;
    esp_err_t result;

    while (1) {
        xEventGroupWaitBits(s_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

        // --- INÍCIO DA SEÇÃO CRÍTICA PARA LEITURA ESTÁVEL ---
        // Cria uma "bolha de proteção" para a leitura do sensor, desabilitando
        // interrupções que possam atrapalhar o timing preciso.
        portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
        portENTER_CRITICAL(&myMutex);

        result = dht_read(DHT11_GPIO_PIN, &reading);

        portEXIT_CRITICAL(&myMutex);
        // --- FIM DA SEÇÃO CRÍTICA ---

        if (result == ESP_OK) {
            float temperature = reading.temperature;
            float humidity = reading.humidity;
            
            float heat_index = calculate_heat_index(temperature, humidity);
            const char* comfort_status = get_comfort_status(temperature);

            ESP_LOGI(TAG, "Leitura: Temp: %.1fC, Umid: %.1f%%, Sensacao: %.1fC, Status: %s", 
                     temperature, humidity, heat_index, comfort_status);

            snprintf(json_payload, sizeof(json_payload),
                     "{\"device_id\":\"%s\",\"temperatura\":%.1f,\"umidade\":%.1f,"
                     "\"indice_calor\":%.1f,\"status_conforto\":\"%s\",\"timestamp\":%lld}",
                     DEVICE_ID, temperature, humidity, heat_index, comfort_status, esp_timer_get_time() / 1000000);

            esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, json_payload, 0, 1, 0);
        } else {
            ESP_LOGE(TAG, "Falha na leitura do sensor DHT11.");
        }

        vTaskDelay(pdMS_TO_TICKS(PUBLISH_INTERVAL_MS));
    }
}

// ============================================================================
// FUNÇÃO PRINCIPAL
// ============================================================================
void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Monitor de Animais v1.0");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    setup_connections();
    xTaskCreate(sensor_publish_task, "sensor_publish_task", 4096, NULL, 5, NULL);
}