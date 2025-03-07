#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"

#include "esp_log.h"

#include "button.h"
#include "epaper.h"
#include "font.h"
#include "http.h"

static const char* TAG = "epaper.c";

#define SPI_CS_PIN 5
#define SPI_MOSI_PIN 23
#define SPI_SCLK_PIN 18
#define DC_PIN 19
#define RESET_PIN 16

#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480
#define DISPLAY_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)

static uint8_t epaper_buffer[DISPLAY_BUFFER_SIZE];
static uint8_t screen_color = SCREEN_WHITE;
static spi_device_handle_t spi_device;

static void spi_init()
{
    gpio_config_t io_conf = { .pin_bit_mask = (1ULL << SPI_CS_PIN) | (1ULL << RESET_PIN) | (1ULL << DC_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE };

    gpio_config(&io_conf);

    // Set CS high; active low
    gpio_set_level(SPI_CS_PIN, 1);

    spi_bus_config_t spi_bus_config = {
        .mosi_io_num = SPI_MOSI_PIN,
        .sclk_io_num = SPI_SCLK_PIN,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
        .flags = 0,
    };

    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = 12000000,
        // Manual CS control through gpio
        .spics_io_num = -1,
        .mode = 0,
        .queue_size = 1,
        .command_bits = 0,
        .address_bits = 0,
        .flags = SPI_DEVICE_NO_DUMMY,
    };

    spi_bus_initialize(SPI2_HOST, &spi_bus_config, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI2_HOST, &dev_config, &spi_device);

    ESP_LOGI(TAG, "SPI initialized.");
}

static inline void cs_select()
{
    asm volatile("nop \n nop \n nop");
    gpio_set_level(SPI_CS_PIN, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect()
{
    asm volatile("nop \n nop \n nop");
    gpio_set_level(SPI_CS_PIN, 1);
    asm volatile("nop \n nop \n nop");
}

static inline void dc_command()
{
    asm volatile("nop \n nop \n nop");
    gpio_set_level(DC_PIN, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void dc_data()
{
    asm volatile("nop \n nop \n nop");
    gpio_set_level(DC_PIN, 1);
    asm volatile("nop \n nop \n nop");
}

static void epaper_reset()
{
    gpio_set_level(RESET_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(RESET_PIN, 1);
}

static void epaper_write_command(uint8_t command)
{
    static uint8_t command_buffer[1];
    command_buffer[0] = command;

    static spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));
    trans.length = 1 * 8; // 1 byte
    trans.tx_buffer = command_buffer;

    cs_select();
    dc_command();

    spi_device_transmit(spi_device, &trans);

    cs_deselect();
}

static void epaper_write_data(uint8_t data)
{
    static uint8_t data_buffer[1];
    data_buffer[0] = data;

    static spi_transaction_t trans;
    memset(&trans, 0, sizeof(trans));
    trans.length = 1 * 8; // 1 byte
    trans.tx_buffer = data_buffer;

    cs_select();
    dc_data();

    spi_device_transmit(spi_device, &trans);

    cs_deselect();
}

static void epaper_check_status()
{
    // TODO: checkstatus
    vTaskDelay(pdMS_TO_TICKS(100));
}

void epaper_init()
{
    memset(epaper_buffer, 0xff, DISPLAY_BUFFER_SIZE);

    spi_init();
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "epaper init...");

    epaper_reset();
    vTaskDelay(pdMS_TO_TICKS(10));

    epaper_check_status();

    epaper_write_command(0x01); // Power setting
    epaper_write_data(0x07);
    epaper_write_data(0x07); // VGH=20V,VGL=-20V
    epaper_write_data(0x3f); // VDH=15V
    epaper_write_data(0x3f); // VDL=-15V

    epaper_write_command(0x06); // Booster soft start
    epaper_write_data(0x17);
    epaper_write_data(0x17);
    epaper_write_data(0x28);
    epaper_write_data(0x17);

    epaper_write_command(0x04); // Power on

    epaper_check_status();

    epaper_write_command(0x00); // Panel setting
    epaper_write_data(0x0f); // KW-3f, KWR-2F, BWROTP 0f, BWOTP 1f

    epaper_write_command(0x61); // Resolution setting
    epaper_write_data(DISPLAY_WIDTH / 256);
    epaper_write_data(DISPLAY_WIDTH % 256);
    epaper_write_data(DISPLAY_HEIGHT / 256);
    epaper_write_data(DISPLAY_HEIGHT % 256);

    epaper_write_command(0x15);
    epaper_write_data(0x00);

    epaper_write_command(0x50); // VCOM and data interval setting
    epaper_write_data(0x11);
    epaper_write_data(0x07);

    epaper_write_command(0x60); // TCON setting
    epaper_write_data(0x22);

    ESP_LOGI(TAG, "epaper init done.");
}

void epaper_init_fast()
{
    memset(epaper_buffer, 0xff, DISPLAY_BUFFER_SIZE);

    spi_init();
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "epaper init (fast)...");

    epaper_reset();
    vTaskDelay(pdMS_TO_TICKS(10));

    epaper_check_status();

    epaper_write_command(0x00); // Panel setting
    epaper_write_data(0x0F); // KW-3f, KWR-2F, BWROTP 0f, BWOTP 1f

    epaper_write_command(0x04); // Power on

    vTaskDelay(pdMS_TO_TICKS(100));

    epaper_check_status();

    // Enhanced display drive(Add 0x06 command)
    epaper_write_command(0x06); // Booster Soft Start
    epaper_write_data(0x27);
    epaper_write_data(0x27);
    epaper_write_data(0x18);
    epaper_write_data(0x17);

    epaper_write_command(0xE0);
    epaper_write_data(0x02);
    epaper_write_command(0xE5);
    epaper_write_data(0x5A);

    epaper_write_command(0x50); // VCOM and data interval setting
    epaper_write_data(0x11);
    epaper_write_data(0x07);

    ESP_LOGI(TAG, "epaper init (fast) done.");
}

static void epaper_update(void)
{
    epaper_write_command(0x12); // Display refresh
    vTaskDelay(pdMS_TO_TICKS(1)); // !!!The delay here is necessary, 200uS at least!!!

    epaper_check_status();
}

void epaper_deep_sleep(void)
{
    epaper_write_command(0x50); // VCOM AND DATA INTERVAL SETTING
    epaper_write_data(0xf7); // WBmode:VBDF 17|D7 VBDW 97 VBDB 57		WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7

    epaper_write_command(0x02); // power off
    epaper_check_status(); // waiting for the electronic paper IC to release the idle signal

    vTaskDelay(10); //!!!The delay here is necessary, 200uS at least!!!

    epaper_write_command(0x07); // deep sleep
    epaper_write_data(0xA5);
}

void epaper_set_pixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
        return;
    }

    if (color == 1) {
        epaper_buffer[y * (DISPLAY_WIDTH / 8) + (x / 8)] |= 0x80u >> (x % 8);
    } else {
        epaper_buffer[y * (DISPLAY_WIDTH / 8) + (x / 8)] &= ~(0x80u >> (x % 8));
    }
}

void epaper_set_pixel_bits_8(uint16_t x, uint16_t y, uint8_t bits)
{
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
        return;
    }

    epaper_buffer[y * (DISPLAY_WIDTH / 8) + (x / 8)] = bits;
}

uint8_t epaper_get_pixel(uint16_t x, uint16_t y)
{
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
        return 0;
    }

    return epaper_buffer[y * (DISPLAY_WIDTH / 8) + (x / 8)] & (0x80u >> (x % 8)) ? 1 : 0;
}

uint8_t epaper_get_pixel_bits_8(uint16_t x, uint16_t y)
{
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
        return 0;
    }

    return epaper_buffer[y * (DISPLAY_WIDTH / 8) + (x / 8)];
}

void epaper_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color)
{
    int16_t dx = abs(x2 - x1);
    int16_t sx = x1 < x2 ? 1 : -1;
    int16_t dy = -abs(y2 - y1);
    int16_t sy = y1 < y2 ? 1 : -1;
    int16_t err = dx + dy;
    int16_t e2;

    while (1) {
        epaper_set_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2)
            break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void epaper_draw_text(uint16_t pos_x, uint16_t pos_y, const char* text, Font* font)
{
    ESP_LOGI(TAG, "draw_text: %s", text);

    for (uint16_t i = 0; i < strlen(text); i++) {
        char char_code = text[i];
        uint16_t char_index = char_code - font->first_char;

        for (uint16_t y = 0; y < font->char_height; y++) {
            for (uint16_t x = 0; x < font->char_width; x++) {
                uint16_t current_bit = char_index * font->char_width + x;
                uint16_t byte_index = y * (font->size / font->char_height) + current_bit / 8;
                uint8_t bit_pos = 7 - (current_bit % 8);
                uint8_t pixel = (font->font_array[byte_index] >> bit_pos) & 0x01;

                // ESP_LOGI("draw_text inner", "current_bit: %d, byte_index: %d, bit_pos: %d, pixel: %d", current_bit, byte_index, bit_pos, pixel);

                epaper_set_pixel(pos_x + i * font->char_width + x, pos_y + y, !pixel);
            }
        }
    }
}

void epaper_dummy_screen()
{
    uint16_t mark = 0x1000;

    memset(epaper_buffer, 0xff, DISPLAY_BUFFER_SIZE);

    epaper_draw_text(20, 20, "Hello, world!", &font_jetbrains_mono_16x24);
    epaper_draw_text(20, 60, "Give me", &font_jetbrains_mono_16x24);
    epaper_draw_text(40, 90, "$1,000,000", &font_jetbrains_mono_16x24);
    epaper_draw_text(20, 120, "please...", &font_jetbrains_mono_16x24);

    epaper_draw_line(20, DISPLAY_HEIGHT / 2, DISPLAY_HEIGHT / 2 - 20, DISPLAY_HEIGHT / 2, 0);

    for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++) {
        for (uint16_t x = DISPLAY_HEIGHT / 16; x < DISPLAY_WIDTH / 8; x += 2) {
            if (mark & 0x1000) {
                epaper_buffer[y * (DISPLAY_WIDTH / 8) + x] = 0xff;
                epaper_buffer[y * (DISPLAY_WIDTH / 8) + x + 1] = 0x00;
            } else {
                epaper_buffer[y * (DISPLAY_WIDTH / 8) + x] = 0x00;
                epaper_buffer[y * (DISPLAY_WIDTH / 8) + x + 1] = 0xff;
            }
        }

        if ((mark & 0xfff) < 10) {
            mark++;
        } else {
            mark = 0 | ((!((mark & 0x1000) >> 12)) << 12);
        }
    }

    if (screen_color == SCREEN_BLACK) {
        epaper_black_screen();
    } else {
        epaper_white_screen();
    }

    epaper_update();
}

void epaper_clear_screen()
{
    memset(epaper_buffer, 0xff, DISPLAY_BUFFER_SIZE);

    if (screen_color == SCREEN_BLACK) {
        epaper_black_screen();
    } else {
        epaper_white_screen();
    }
}

void epaper_white_screen()
{
    epaper_write_command(0x10);
    for (uint32_t i = 0; i < DISPLAY_BUFFER_SIZE; i++) {
        epaper_write_data(epaper_buffer[i]);
    }

    epaper_write_command(0x13);
    for (uint32_t i = 0; i < DISPLAY_BUFFER_SIZE; i++) {
        epaper_write_data(0x00);
    }

    epaper_update();

    screen_color = SCREEN_WHITE;
}

void epaper_black_screen()
{
    epaper_write_command(0x10);
    for (uint32_t i = 0; i < DISPLAY_BUFFER_SIZE; i++) {
        epaper_write_data(~epaper_buffer[i]);
    }

    epaper_write_command(0x13);
    for (uint32_t i = 0; i < DISPLAY_BUFFER_SIZE; i++) {
        epaper_write_data(0x00);
    }

    epaper_update();

    screen_color = SCREEN_BLACK;
}

void epaper_toggle_screen_color()
{
    if (screen_color == SCREEN_WHITE) {
        epaper_black_screen();
    } else {
        epaper_white_screen();
    }
}

static void epaper_button1_press_cb()
{
    epaper_clear_screen();
    epaper_deep_sleep();
}

static void epaper_button2_press_cb()
{
    epaper_toggle_screen_color();
    epaper_deep_sleep();
}

static void epaper_button3_press_cb()
{
    epaper_dummy_screen();
    epaper_deep_sleep();
}

static void epaper_draw_text_cb(const char* text, int x, int y)
{
    epaper_draw_text(x, y, text, &font_jetbrains_mono_16x24);

    if (screen_color == SCREEN_BLACK) {
        epaper_black_screen();
    } else {
        epaper_white_screen();
    }

    epaper_update();
    epaper_deep_sleep();
}

void epaper_setup()
{
    epaper_init_fast();

    button_register_button1_press_cb(epaper_button1_press_cb);
    button_register_button2_press_cb(epaper_button2_press_cb);
    button_register_button3_press_cb(epaper_button3_press_cb);

    http_register_on_draw_text(epaper_draw_text_cb);
}
