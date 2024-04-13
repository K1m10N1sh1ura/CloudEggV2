#include "server.h"
#include "cJSON.h"
#include "main.h"

// HTML-Seite als Konstante
const char* html_response = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <title>ESP32 Web Server</title>\n"
"</head>\n"
"<body>\n"
"    <h1>ESP32 Web Server</h1>\n"
"    <button id='actionButton' onclick='startMeasurement()'>Starte Messung</button>\n"
"    <p>Zeitwert: <span id='timeValue'> 0.00 s </span></p>\n"
"    <p>Status: <span id='statusMessage'> Bereit...</span></p>\n"
"\n"
"    <script>\n"
"    var intervalId = null; // ID des Intervalls\n"
"\n"
"    function startMeasurement() {\n"
"        document.getElementById('actionButton').disabled = true; // Button deaktivieren\n"
"        if (intervalId) {\n"
"            clearInterval(intervalId); // Sicherstellen, dass keine vorherige Intervalle laufen\n"
"        }\n"
"        fetchStartMeasurement();\n"
"        intervalId = setInterval(fetchTimeValue, 200); // Intervall starten\n"
"    }\n"
"\n"
"    function fetchTimeValue() {\n"
"        fetch('/getTimeAndStatus')\n"
"            .then(response => {\n"
"                if (!response.ok) {\n"
"                    throw new Error('Server response not OK');\n"
"                }\n"
"                return response.json();\n"
"            })\n"
"            .then(data => {\n"
"               document.getElementById('timeValue').textContent = data.timeValue;\n"
"               document.getElementById('statusMessage').textContent = 'Messung aktiv...!';\n"
"               if (!data.runningMeasurement) {\n"
"                    stopMeasurement(); // Messung stoppen, wenn nicht mehr aktiv\n"
"                }\n"
"            })\n"
"            .catch(err => {\n"
"                console.error('Fehler bei der Anfrage:', err);\n"
"                document.getElementById('statusMessage').textContent = 'Fehler!';\n"
"                stopMeasurement(); // Stoppen, wenn ein Fehler auftritt\n"
"            });\n"
"    }\n"
"    function fetchStartMeasurement() {\n"
"        fetch('/startMeas')\n"
"            .then(response => {\n"
"                if (!response.ok) {\n"
"                    throw new Error('Server response not OK');\n"
"                }\n"
"                return response.text();\n"
"            })\n"
"            .then(data => {\n"
"                document.getElementById('statusMessage').textContent = data;\n"
"            })\n"
"            .catch(err => {\n"
"                console.error('Fehler bei der Anfrage:', err);\n"
"                document.getElementById('statusMessage').textContent = 'Messung konnte nicht gestartet werden!';\n"
"                stopMeasurement(); // Stoppen, wenn ein Fehler auftritt\n"
"            });\n"
"    }\n"
"\n"
"    function stopMeasurement() {\n"
"        clearInterval(intervalId); // Intervall stoppen\n"
"        document.getElementById('actionButton').disabled = false; // Button reaktivieren\n"
"        document.getElementById('statusMessage').textContent = 'Bereit...!';\n"
"    }\n"
"    </script>\n"
"</body>\n"
"</html>";

// HTTP GET Handler
static esp_err_t get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/*static esp_err_t get_time_and_status_handler(httpd_req_t *req) {
    if (meastask_manager.running_measurement) {
        char buf[8];
        int64_t time_value = meastask_manager.current_time_us; 
        sprintf(buf, "%.2f", (float) time_value / (float) 1e6);
        httpd_resp_sendstr(req, buf);
    } 
    
    return ESP_OK;
}*/
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
    char buf[64];
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
    .user_ctx = NULL
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