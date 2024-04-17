#include "server.h"
// HTTP GET Handler

WEBKIT_RESPONSE_ARGS webkit_upload_args = { index_html, &index_html_len };

static esp_err_t get_handler(httpd_req_t *req) {
    /* Send response with custom headers and body set as the
     * string passed in user context*/
    WEBKIT_RESPONSE_ARGS* args = (WEBKIT_RESPONSE_ARGS *)(req->user_ctx);
    
    httpd_resp_set_type(req, HTTPD_TYPE_TEXT);    
    httpd_resp_send(req, (const char*)args->resp, *(args->resp_len));
    return ESP_OK;
}

static esp_err_t get_time_and_status_handler(httpd_req_t *req) {
    static char buf[8];
    cJSON *root = cJSON_CreateObject();

    int64_t time_value = meastask_manager.current_time_us; 
    sprintf(buf, "%.2f", (float) time_value / (float) 1e6);
    cJSON_AddStringToObject(root, "timeValue", buf);
    cJSON_AddBoolToObject(root, "runningMeasurement", meastask_manager.running_measurement);

    const char *json_string = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_string);

    cJSON_Delete(root);
    free((void *)json_string);

    return ESP_OK;
}

static esp_err_t start_meas_handler(httpd_req_t *req) {
    static char buf[64];
    if (!meastask_manager.running_measurement) {
        vTaskResume(xAcousticBarrierTaskHandle); // Resume the acoustic barrier task
        sprintf(buf, "Messung gestartet!");
    }
    else {
        sprintf(buf, "Messung konnte nicht gestartet werden!");
    }
    httpd_resp_sendstr(req, buf);

    return ESP_OK;
}

httpd_uri_t uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx  = (void *)&webkit_upload_args 
};

httpd_uri_t get_time_and_status_uri = {
    .uri       = "/getTimeAndStatus",
    .method    = HTTP_GET,
    .handler   = get_time_and_status_handler,
    .user_ctx  = NULL
};

httpd_uri_t start_meas_uri = {
    .uri       = "/startMeas",
    .method    = HTTP_GET,
    .handler   = start_meas_handler,
    .user_ctx  = NULL
};

/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &start_meas_uri);
        httpd_register_uri_handler(server, &get_time_and_status_uri);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

/* Function for stopping the webserver */
void stop_webserver(httpd_handle_t server)
{
    if (server) {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}

void server_init() {
    static httpd_handle_t server = NULL;  
    server = start_webserver();
    if (server == NULL) {
        printf("Webserver init failed\n");
    }
    else {
        printf("Webserver started\n");
    }
}