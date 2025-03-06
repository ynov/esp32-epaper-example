#include "driver/gpio.h"

#include "esp_attr.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "button.h"

#define BUTTON_SW_DEBOUNCE_MS 300

static const char* TAG = "button.c";

static QueueHandle_t button_press_queue;

static void (*on_button1_press)(void);
static void (*on_button2_press)(void);
static void (*on_button3_press)(void);

void button_register_button1_press_cb(void (*callback)(void))
{
    on_button1_press = callback;
}

void button_register_button2_press_cb(void (*callback)(void))
{
    on_button2_press = callback;
}

void button_register_button3_press_cb(void (*callback)(void))
{
    on_button3_press = callback;
}

static void noop() {

}

static void IRAM_ATTR button_press_handler(void* args)
{
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = xTaskGetTickCountFromISR();
    uint32_t button_id = (uint32_t) args;

    if (interrupt_time - last_interrupt_time > pdMS_TO_TICKS(BUTTON_SW_DEBOUNCE_MS)) {
        last_interrupt_time = interrupt_time;

        xQueueSendFromISR(button_press_queue, &button_id, NULL);
    }
}

void button_init(void)
{
    button_press_queue = xQueueCreate(10, 4 * sizeof(uint32_t));

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE, // Interrupt on falling edge
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON1_PIN) | (1ULL << BUTTON2_PIN) | (1ULL << BUTTON3_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };

    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    gpio_isr_handler_add(BUTTON1_PIN, button_press_handler, (void*) BUTTON1_PIN);
    gpio_isr_handler_add(BUTTON2_PIN, button_press_handler, (void*) BUTTON2_PIN);
    gpio_isr_handler_add(BUTTON3_PIN, button_press_handler, (void*) BUTTON3_PIN);

    button_register_button1_press_cb(noop);
    button_register_button2_press_cb(noop);
    button_register_button3_press_cb(noop);

    ESP_LOGI(TAG, "button event handlers initialized.");
}

void button_task(void* parameters)
{
    uint8_t button_id;

    button_init();

    while (true) {

        if (xQueueReceive(button_press_queue, &button_id, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "Button %d pressed", button_id);

            if (button_id == BUTTON1_PIN) {
                on_button1_press();
            } else if (button_id == BUTTON2_PIN) {
                on_button2_press();
            } else if (button_id == BUTTON3_PIN) {
                on_button3_press();
            }

        }

        vTaskDelay(10);
    }
}

void button_create_task(TaskHandle_t* handle)
{
    xTaskCreate(button_task, "button_task", 4096, NULL, 1, handle);
}
