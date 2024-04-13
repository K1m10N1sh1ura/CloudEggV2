#include "server.h"
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
"    <button id='actionButton' onclick='startMeasurement()'>Start Messung</button>\n"
"    <p>Zeitwert: <span id='timeValue'>Warte auf Messung...</span></p>\n"
"    <p id='statusMessage'></p> <!-- Platzhalter für Statusmeldungen -->\n"
"\n"
"    <script>\n"
"    var intervalId = null; // ID des Intervalls\n"
"\n"
"    function startMeasurement() {\n"
"        document.getElementById('actionButton').disabled = true; // Button deaktivieren\n"
"        document.getElementById('statusMessage').textContent = ''; // Statusmeldung zurücksetzen\n"
"        if (intervalId) {\n"
"            clearInterval(intervalId); // Sicherstellen, dass keine vorherige Intervalle laufen\n"
"        }\n"
"        intervalId = setInterval(fetchTimeValue, 100); // Intervall starten\n"
"    }\n"
"\n"
"    function fetchTimeValue() {\n"
"        fetch('/getTime')\n"
"            .then(response => {\n"
"                if (!response.ok) {\n"
"                    throw new Error('Server response not OK');\n"
"                }\n"
"                return response.text();\n"
"            })\n"
"            .then(data => {\n"
"                document.getElementById('timeValue').textContent = data;\n"
"            })\n"
"            .catch(err => {\n"
"                console.error('Fehler bei der Anfrage:', err);\n"
"                document.getElementById('statusMessage').textContent = 'Messung beendet!'; // Nachricht anzeigen\n"
"                stopMeasurement(); // Stoppen, wenn ein Fehler auftritt\n"
"            });\n"
"    }\n"
"\n"
"    function stopMeasurement() {\n"
"        clearInterval(intervalId); // Intervall stoppen\n"
"        document.getElementById('actionButton').disabled = false; // Button reaktivieren\n"
"    }\n"
"    </script>\n"
"</body>\n"
"</html>";

// HTTP GET Handler
esp_err_t get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_time_handler(httpd_req_t *req) {
    static bool ready_state = true;
    static int64_t time_value = 0;
    time_value = meastask_manager.current_time_us; 
    if (!meastask_manager.running_measurement && ready_state) {
        vTaskResume(xAcousticBarrierTaskHandle); // Resume the acoustic barrier task
        ready_state = false;
    } 
    else if (!ready_state && !meastask_manager.running_measurement) {
        ready_state = true;
    }
    if (meastask_manager.running_measurement) {
        char buf[64];
        sprintf(buf, "%.2f", (float) time_value / (float) 1e6);
        httpd_resp_sendstr(req, buf);
    }
    return ESP_OK;
}

httpd_uri_t uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

httpd_uri_t get_time_uri = {
    .uri       = "/getTime",
    .method    = HTTP_GET,
    .handler   = get_time_handler,
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
        httpd_register_uri_handler(server, &get_time_uri);
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
}