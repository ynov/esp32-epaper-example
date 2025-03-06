#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"

#include "epaper.h"
#include "page/index.html.h"
#include "font.h"

static const char* TAG = "http.c";

static void (*on_draw_text)(const char* text, int x, int y);

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

/**
 * TODO: using JSON here is overkill, we should just use simple POST parameters or query string
 *
 * Example JSON Body:
 *
 * ```json
 * {
 *   "text": "Hello, World!",
 *   "x": 20,
 *   "y": 20
 * }
 * ```
 */
static esp_err_t draw_text_http_handler(httpd_req_t* req)
{
    int content_length = req->content_len;
    if (content_length <= 0) {
        return ESP_FAIL;
    }

    char* content = malloc(content_length + 1);
    if (!content) {
        return ESP_FAIL;
    }

    int received = httpd_req_recv(req, content, content_length);
    if (received != content_length) {
        free(content);
        return ESP_FAIL;
    }
    content[content_length] = '\0';

    cJSON* root = cJSON_Parse(content);
    free(content);

    if (!root) {
        return ESP_FAIL;
    }

    cJSON* text_json = cJSON_GetObjectItem(root, "text");
    cJSON* x_json = cJSON_GetObjectItem(root, "x");
    cJSON* y_json = cJSON_GetObjectItem(root, "y");

    if (!text_json || !text_json->valuestring ||
        !x_json || (!x_json->valueint && x_json->valueint != 0) ||
        !y_json || (!y_json->valueint && y_json->valueint != 0)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "draw_text_http_handler: %s, %d, %d", text_json->valuestring, x_json->valueint, y_json->valueint);
    on_draw_text(text_json->valuestring, x_json->valueint, y_json->valueint);

    cJSON_Delete(root);

    const char resp[] = "OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

void http_register_on_draw_text(void (*callback)(const char* text, int x, int y))
{
    on_draw_text = callback;
}

static void noop_draw_text(const char* text, int x, int y)
{
    ESP_LOGI(TAG, "noop_draw_text");
}

void http_server_init()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    http_register_on_draw_text(noop_draw_text);

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
        httpd_uri_t draw_text_uri = {
            .uri = "/draw_text",
            .method = HTTP_POST,
            .handler = draw_text_http_handler,
            .user_ctx = NULL
        };

        httpd_register_uri_handler(server, &root_uri);
        httpd_register_uri_handler(server, &toggle_screen_color_uri);
        httpd_register_uri_handler(server, &clear_screen_uri);
        httpd_register_uri_handler(server, &dummy_screen_uri);
        httpd_register_uri_handler(server, &draw_text_uri);
    }
}
