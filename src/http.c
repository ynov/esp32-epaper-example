#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "epaper.h"
#include "page/index.html.h"

static const char* TAG = "http.c";

static esp_err_t root_http_handler(httpd_req_t* req)
{
    httpd_resp_send(req, (const char*) index_html, index_html_len);

    ESP_LOGI(TAG, "root_http_handler");

    return ESP_OK;
}

static esp_err_t toggle_screen_color_http_handler(httpd_req_t* req)
{
    const char resp[] = "OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    epaper_toggle_screen_color();
    epaper_deep_sleep();

    ESP_LOGI(TAG, "toggle_screen_color_http_handler");

    return ESP_OK;
}

static esp_err_t clear_screen_http_handler(httpd_req_t* req)
{
    const char resp[] = "OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    epaper_clear_screen();
    epaper_deep_sleep();

    ESP_LOGI(TAG, "clear_screen_http_handler");

    return ESP_OK;
}

static esp_err_t dummy_screen_http_handler(httpd_req_t* req)
{
    const char resp[] = "OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    epaper_dummy_screen();
    epaper_deep_sleep();

    return ESP_OK;
}

void http_server_init()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = root_http_handler,
            .user_ctx = NULL
        };
        httpd_uri_t toggle_screen_color_uri = {
            .uri = "/toggle_screen_color",
            .method = HTTP_GET,
            .handler = toggle_screen_color_http_handler,
            .user_ctx = NULL
        };
        httpd_uri_t clear_screen_uri = {
            .uri = "/clear_screen",
            .method = HTTP_GET,
            .handler = clear_screen_http_handler,
            .user_ctx = NULL
        };
        httpd_uri_t dummy_screen_uri = {
            .uri = "/dummy_screen",
            .method = HTTP_GET,
            .handler = dummy_screen_http_handler,
            .user_ctx = NULL
        };

        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &toggle_screen_color_uri);
        httpd_register_uri_handler(server, &clear_screen_uri);
        httpd_register_uri_handler(server, &dummy_screen_uri);
    }
}
