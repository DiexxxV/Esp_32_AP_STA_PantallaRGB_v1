#include "web_server.h"
#include <string.h>
#include <stdlib.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "cJSON.h"

#include "wifi_mgr.h"
#include "io_ctrl.h"
#include "system_state.h"
#include "nvs_storage.h"

#define TAG "web_server"

/* ================= HTML ================= */

static const char index_html[] =
"<!doctype html><html lang='en'><head>"
"<meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1'>"
"<title>ESP32 Control</title>"
"<style>"
"body{font-family:sans-serif;margin:0;padding:20px;background:#f5f5f5;}"
"h2,h3{color:#333;}"
".section{margin-bottom:20px;padding:15px;background:#fff;border-radius:8px;"
"box-shadow:0 0 5px rgba(0,0,0,0.1);}"
"button{padding:12px 20px;margin:5px;border:none;border-radius:6px;"
"font-size:16px;cursor:pointer;}"
".on{background:#28a745;color:white;}"
".off{background:#dc3545;color:white;}"
".wifi-net{margin:5px 0;padding:5px;background:#eee;border-radius:5px;"
"display:flex;justify-content:space-between;align-items:center;}"
"input{padding:8px;width:70%;border-radius:5px;border:1px solid #ccc;}"
"@media(max-width:600px){button{width:100%;margin:5px 0;}}"
"</style></head><body>"

"<h2>ESP32 WiFi Control</h2>"
"<p><b>Status:</b> <span id='sys'>-</span></p>"
"<p><b>STA IP:</b> <span id='ip'>-</span></p>"

"<div class='section'>"
"<h3>WiFi Networks</h3>"
"<button onclick='scan()'>Scan</button>"
"<ul id='nets'></ul>"
"<input id='pass' type='password' placeholder='WiFi Password'>"
"</div>"

"<div class='section'>"
"<h3>Relays</h3>"
"<button id='r1' class='off' onclick='relay(1)'>Relay 1 OFF</button>"
"<button id='r2' class='off' onclick='relay(2)'>Relay 2 OFF</button>"
"</div>"

"<div class='section'>"
"<h3>Inputs</h3>"
"<p>Input 1: <span id='in1'>0</span></p>"
"<p>Input 2: <span id='in2'>0</span></p>"
"</div>"

"<div class='section'>"
"<h3>System</h3>"
"<button onclick='resetWifi()'>Reset WiFi</button>"
"</div>"

"<script>"
"function scan(){"
" fetch('/scan').then(r=>r.json()).then(j=>{"
"  nets.innerHTML='';"
"  j.forEach(n=>{"
"   let li=document.createElement('li');"
"   li.className='wifi-net';"
"   let btn=document.createElement('button');"
"   btn.textContent='Connect';"
"   btn.onclick=()=>connect(n.ssid);"
"   li.appendChild(document.createTextNode(n.ssid+' ('+n.rssi+') '));"
"   li.appendChild(btn);"
"   nets.appendChild(li);"
"  });"
" });"
"}"

"function connect(ssid){"
" fetch('/connect',{method:'POST',headers:{'Content-Type':'application/json'},"
" body:JSON.stringify({ssid:ssid,pass:pass.value})});"
"}"

"function relay(num){"
" let btn=document.getElementById('r'+num);"
" let state = btn.classList.contains('on') ? 0 : 1;"
" fetch('/relay',{method:'POST',headers:{'Content-Type':'application/json'},"
" body:JSON.stringify({relay:num,state:state})});"
" btn.className = state ? 'on':'off';"
" btn.textContent='Relay '+num+' '+(state?'ON':'OFF');"
"}"

"function resetWifi(){"
" fetch('/reset_wifi',{method:'POST'});"
" alert('WiFi settings cleared, reboot device');"
"}"

"setInterval(()=>{"
" fetch('/status').then(r=>r.json()).then(s=>{"
"   sys.textContent = s.system;"
"   ip.textContent = s.wifi.ip;"
" });"
" fetch('/inputs').then(r=>r.json()).then(j=>{"
"   in1.textContent=j.in1;"
"   in2.textContent=j.in2;"
" });"
"},1000);"
"</script></body></html>";

/* ================= UTIL ================= */

static esp_err_t read_post_data(httpd_req_t *req, char *buf, size_t max)
{
    int total = req->content_len;
    int cur = 0;
    int received;

    if (total >= max) return ESP_FAIL;

    while (cur < total) {
        received = httpd_req_recv(req, buf + cur, total - cur);
        if (received <= 0) return ESP_FAIL;
        cur += received;
    }

    buf[cur] = 0;
    return ESP_OK;
}

/* ================= HANDLERS ================= */

static esp_err_t index_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t scan_get(httpd_req_t *req)
{
    static char buf[1024];
    wifi_scan(buf, sizeof(buf));
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t connect_post(httpd_req_t *req)
{
    char buf[256];
    if (read_post_data(req, buf, sizeof(buf)) != ESP_OK) return ESP_FAIL;

    cJSON *root = cJSON_Parse(buf);
    if (!root) return ESP_FAIL;

    cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *pass = cJSON_GetObjectItem(root, "pass");

    if (!cJSON_IsString(ssid) || !cJSON_IsString(pass)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    system_state_set(SYS_CONNECTING_STA);
    wifi_connect_sta(ssid->valuestring, pass->valuestring);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static esp_err_t relay_post(httpd_req_t *req)
{
    char buf[128];
    if (read_post_data(req, buf, sizeof(buf)) != ESP_OK) return ESP_FAIL;

    cJSON *root = cJSON_Parse(buf);
    if (!root) return ESP_FAIL;

    cJSON *relay = cJSON_GetObjectItem(root, "relay");
    cJSON *state = cJSON_GetObjectItem(root, "state");

    if (!cJSON_IsNumber(relay) || !cJSON_IsNumber(state)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    io_set_relay(relay->valueint, state->valueint);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

static esp_err_t inputs_get(httpd_req_t *req)
{
    char resp[64];
    snprintf(resp, sizeof(resp),
        "{\"in1\":%d,\"in2\":%d}",
        io_get_input(1),
        io_get_input(2)
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

static esp_err_t status_get(httpd_req_t *req)
{
    char resp[256];

    system_state_t st = system_state_get();
    const char *ip = wifi_get_sta_ip();

    snprintf(resp, sizeof(resp),
        "{"
        "\"system\":\"%s\","
        "\"wifi\":{\"connected\":%s,\"ip\":\"%s\"},"
        "\"uptime\":%lld"
        "}",
        system_state_to_str(st),
        (st == SYS_STA_CONNECTED || st == SYS_RUNNING) ? "true" : "false",
        ip ? ip : "0.0.0.0",
        esp_timer_get_time() / 1000000
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

static esp_err_t reset_wifi_post(httpd_req_t *req)
{
    ESP_LOGW(TAG, "Resetting WiFi credentials");
    nvs_clear_wifi();
    system_state_set(SYS_AP_MODE);
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

/* ================= SERVER ================= */

static httpd_handle_t server = NULL;

void web_server_start(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size = 4096;

    if (httpd_start(&server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server");
        return;
    }

    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/",          .method=HTTP_GET,  .handler=index_get });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/scan",      .method=HTTP_GET,  .handler=scan_get });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/connect",   .method=HTTP_POST, .handler=connect_post });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/relay",     .method=HTTP_POST, .handler=relay_post });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/inputs",    .method=HTTP_GET,  .handler=inputs_get });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/status",    .method=HTTP_GET,  .handler=status_get });
    httpd_register_uri_handler(server, &(httpd_uri_t){ .uri="/reset_wifi",.method=HTTP_POST, .handler=reset_wifi_post });

    ESP_LOGI(TAG, "Web server started");

    /* ==== AUTO CONNECT FROM NVS ==== */
    char ssid[32] = {0};
    char pass[64] = {0};

	if (nvs_load_wifi(ssid, sizeof(ssid), pass, sizeof(pass))) {

        ESP_LOGI(TAG, "Auto connecting to saved WiFi: %s", ssid);
        system_state_set(SYS_CONNECTING_STA);
        wifi_connect_sta(ssid, pass);
    } else {
        ESP_LOGI(TAG, "No saved WiFi, staying in AP mode");
    }
}
