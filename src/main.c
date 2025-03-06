#include "button.h"
#include "epaper.h"
#include "http.h"
#include "wifi.h"

void app_main(void)
{
    wifi_task_params wifi_params = {
        .on_init_success = http_server_init,
    };
    wifi_create_task(&wifi_params, NULL);
    button_create_task(NULL);

    epaper_setup();
}
