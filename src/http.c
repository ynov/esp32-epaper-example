#include "esp_http_server.h"
#include "esp_log.h"

#define SCREEN_WHITE 0
#define SCREEN_BLACK 1

extern void epaper_white_screen();
extern void epaper_black_screen();
extern void epaper_deep_sleep();

extern volatile uint8_t screen;
extern volatile uint32_t counter;

extern const char* page_content;
static const char* TAG = "http.c";

esp_err_t root_http_handler(httpd_req_t* req)
{
    httpd_resp_send(req, page_content, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

esp_err_t counter_http_handler(httpd_req_t* req)
{
    char resp[256];

    sprintf(resp, "%d", (int) counter);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

esp_err_t toggle_screen_http_handler(httpd_req_t* req)
{
    const char resp[] = "OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    if (screen == SCREEN_WHITE) {
        ESP_LOGI(TAG, "Toggle screen to black");
        epaper_black_screen();
        epaper_deep_sleep();
        screen = SCREEN_BLACK;
    } else {
        ESP_LOGI(TAG, "Toggle screen to white");
        epaper_white_screen();
        epaper_deep_sleep();
        screen = SCREEN_WHITE;
    }

    return ESP_OK;
}

void http_server_init()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_http_handler, .user_ctx = NULL };
        httpd_uri_t toggle_screen_uri
            = { .uri = "/toggle_screen", .method = HTTP_GET, .handler = toggle_screen_http_handler, .user_ctx = NULL };
        httpd_uri_t counter_uri
            = { .uri = "/counter", .method = HTTP_GET, .handler = counter_http_handler, .user_ctx = NULL };

        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &toggle_screen_uri);
        httpd_register_uri_handler(server, &counter_uri);
    }
}
