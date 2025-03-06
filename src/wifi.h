#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef __WIFI_H
#define __WIFI_H

typedef struct wifi_task_params {
    void (*on_init_success)();
} wifi_task_params;

void wifi_create_task(wifi_task_params* params, TaskHandle_t* handle);

#endif
