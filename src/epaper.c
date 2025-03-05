#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"

#include "esp_log.h"

#include "font.h"

static const char* TAG = "epaper.c";

#define SPI_CS_PIN 5
#define SPI_MOSI_PIN 23
#define SPI_SCLK_PIN 18

#define DC_PIN 19
#define RESET_PIN 16

#define EPD_WIDTH 800
#define EPD_HEIGHT 480
#define EPD_ARRAY_SIZE (EPD_WIDTH * EPD_HEIGHT / 8)

uint8_t epaper_buffer[EPD_ARRAY_SIZE];
spi_device_handle_t spi_device;

void spi_init()
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
        .clock_speed_hz = 10000000,
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

void dc_command()
{
    asm volatile("nop \n nop \n nop");
    gpio_set_level(DC_PIN, 0);
    asm volatile("nop \n nop \n nop");
}

void dc_data()
{
    asm volatile("nop \n nop \n nop");
    gpio_set_level(DC_PIN, 1);
    asm volatile("nop \n nop \n nop");
}

void epaper_reset()
{
    gpio_set_level(RESET_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(RESET_PIN, 1);
}

void epaper_write_command(uint8_t command)
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

void epaper_write_data(uint8_t data)
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

void epaper_check_status()
{
    // TODO: checkstatus
    vTaskDelay(pdMS_TO_TICKS(250));
}

void epaper_init()
{
    memset(epaper_buffer, 0xff, EPD_ARRAY_SIZE);

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

    epaper_write_command(0x00); // Pannel setting
    epaper_write_data(0x0f); // KW-3f, KWR-2F, BWROTP 0f, BWOTP 1f

    epaper_write_command(0x61); // Resolution setting
    epaper_write_data(EPD_WIDTH / 256);
    epaper_write_data(EPD_WIDTH % 256);
    epaper_write_data(EPD_HEIGHT / 256);
    epaper_write_data(EPD_HEIGHT % 256);

    epaper_write_command(0x15);
    epaper_write_data(0x00);

    epaper_write_command(0x50); // VCOM and data interval setting
    epaper_write_data(0x11);
    epaper_write_data(0x07);

    epaper_write_command(0x60); // TCON setting
    epaper_write_data(0x22);

    ESP_LOGI(TAG, "epaper init done.");
}

void epaper_update(void)
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
    if (x >= EPD_WIDTH || y >= EPD_HEIGHT) {
        return;
    }

    if (color == 1) {
        epaper_buffer[y * (EPD_WIDTH / 8) + (x / 8)] |= 0x80u >> (x % 8);
    } else {
        epaper_buffer[y * (EPD_WIDTH / 8) + (x / 8)] &= ~(0x80u >> (x % 8));
    }
}

void epaper_set_pixel_byte(uint16_t x, uint16_t y, uint8_t byte)
{
    if (x >= EPD_WIDTH || y >= EPD_HEIGHT) {
        return;
    }

    epaper_buffer[y * (EPD_WIDTH / 8) + (x / 8)] = byte;
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
    ESP_LOGI(TAG, "draw_text: %s", font->font_array);
    for (uint16_t i = 0; i < strlen(text); i++) {
        char char_code = text[i];
        uint16_t char_index = char_code - font->first_char;

        ESP_LOGI("draw_text", "char_code: %c/%d, char_index: %d", char_code, (int) char_code, char_index);

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

    memset(epaper_buffer, 0xff, EPD_ARRAY_SIZE);

    epaper_draw_text(20, 20, "Hello, world!", &font_jetbrains_mono_16x24);
    epaper_draw_text(20, 60, "Give me", &font_jetbrains_mono_16x24);
    epaper_draw_text(40, 90, "$1,000,000", &font_jetbrains_mono_16x24);
    epaper_draw_text(20, 120, "please...", &font_jetbrains_mono_16x24);

    // for (uint16_t y = 20; y < EPD_HEIGHT / 2 - 20; y++) {
    //     epaper_set_pixel(y, y, 0);
    //     epaper_set_pixel((EPD_HEIGHT / 2) - y, y, 0);
    //     epaper_set_pixel((EPD_HEIGHT / 2) - y, (EPD_HEIGHT / 2) + y, 0);
    //     epaper_set_pixel(y, (EPD_HEIGHT / 2) + y, 0);
    // }

    epaper_draw_line(20, EPD_HEIGHT / 2, EPD_HEIGHT / 2 - 20, EPD_HEIGHT / 2, 0);

    for (uint16_t y = 0; y < EPD_HEIGHT; y++) {
        for (uint16_t x = EPD_HEIGHT / 16; x < EPD_WIDTH / 8; x += 2) {
            if (mark & 0x1000) {
                epaper_buffer[y * (EPD_WIDTH / 8) + x] = 0xff;
                epaper_buffer[y * (EPD_WIDTH / 8) + x + 1] = 0x00;
            } else {
                epaper_buffer[y * (EPD_WIDTH / 8) + x] = 0x00;
                epaper_buffer[y * (EPD_WIDTH / 8) + x + 1] = 0xff;
            }
        }

        if ((mark & 0xfff) < 10) {
            mark++;
        } else {
            mark = 0 | ((!((mark & 0x1000) >> 12)) << 12);
        }
    }

    epaper_write_command(0x10);
    for (uint32_t i = 0; i < EPD_ARRAY_SIZE; i++) {
        epaper_write_data(epaper_buffer[i]);
    }

    epaper_write_command(0x13);
    for (uint32_t i = 0; i < EPD_ARRAY_SIZE; i++) {
        epaper_write_data(0x00);
    }

    epaper_update();
}

void epaper_white_screen()
{
    epaper_write_command(0x10);
    for (uint32_t i = 0; i < EPD_ARRAY_SIZE; i++) {
        epaper_write_data(epaper_buffer[i]);
    }

    epaper_write_command(0x13);
    for (uint32_t i = 0; i < EPD_ARRAY_SIZE; i++) {
        epaper_write_data(0x00);
    }

    epaper_update();
}

void epaper_black_screen()
{
    epaper_write_command(0x10);
    for (uint32_t i = 0; i < EPD_ARRAY_SIZE; i++) {
        epaper_write_data(~epaper_buffer[i]);
    }

    epaper_write_command(0x13);
    for (uint32_t i = 0; i < EPD_ARRAY_SIZE; i++) {
        epaper_write_data(0x00);
    }

    epaper_update();
}
