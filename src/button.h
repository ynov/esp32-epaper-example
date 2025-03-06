#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef __BUTTON_H
#define __BUTTON_H

#define BUTTON1_PIN 34
#define BUTTON2_PIN 35
#define BUTTON3_PIN 39

#define ESP_INTR_FLAG_DEFAULT 0

void button_create_task(TaskHandle_t* handle);
void button_register_button1_press_cb(void (*callback)(void));
void button_register_button2_press_cb(void (*callback)(void));
void button_register_button3_press_cb(void (*callback)(void));

#endif
