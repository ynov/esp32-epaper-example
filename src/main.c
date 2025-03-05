#include "esp_event.h"
#include "esp_https_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>

#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "private.h"

#define EXAMPLE_ESP_WIFI_CHANNEL 1
#define EXAMPLE_MAX_STA_CONN 2

#define BUTTON1_PIN 34
#define BUTTON2_PIN 35

#define ESP_INTR_FLAG_DEFAULT 0

#define SCREEN_WHITE 0
#define SCREEN_BLACK 1

extern void http_server_init();

extern void epaper_init();
extern void epaper_dummy_screen();
extern void epaper_deep_sleep();
static const char* TAG = "main.c";

uint32_t counter = 0;
volatile uint8_t screen = SCREEN_WHITE;

static void IRAM_ATTR button1_click_handler(void* arg)
{
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = xTaskGetTickCountFromISR();

    // Software debounce - ignore interrupts within 500ms
    if (interrupt_time - last_interrupt_time > pdMS_TO_TICKS(500)) {
        last_interrupt_time = interrupt_time;
        counter = 0;
    }
}

static void IRAM_ATTR button2_click_handler(void* arg)
{
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = xTaskGetTickCountFromISR();

    // Software debounce - ignore interrupts within 500ms
    if (interrupt_time - last_interrupt_time > pdMS_TO_TICKS(500)) {
        last_interrupt_time = interrupt_time;
        counter++;
    }
}

// GPIO initialization function
static void button_init(void)
{
    // Configure button GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE, // Interrupt on falling edge
        .mode = GPIO_MODE_INPUT, // Set as input
        .pin_bit_mask = (1ULL << BUTTON1_PIN) | (1ULL << BUTTON2_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE, // Disable pullup
        .pull_down_en = GPIO_PULLDOWN_DISABLE // Enable pulldown
    };

    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpio_isr_handler_add(BUTTON1_PIN, button1_click_handler, NULL);
    gpio_isr_handler_add(BUTTON2_PIN, button2_click_handler, NULL);

    ESP_LOGI(TAG, "Button interrupt initialized.");
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d", MAC2STR(event->mac), event->aid, event->reason);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                .required = true,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d", EXAMPLE_ESP_WIFI_SSID,
        EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    epaper_init();

    vTaskDelay(pdMS_TO_TICKS(100));
    epaper_dummy_screen();
    epaper_deep_sleep();

    button_init();

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();

    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_LOGI("http", "Starting HTTP Server.");

    http_server_init();
}
