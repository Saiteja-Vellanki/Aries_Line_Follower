/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <esp_http_server.h>
#include <lwip/sockets.h>
#include <lwip/api.h>
#include <lwip/netdb.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define ARIES_WIFI_SSID            "Aries_LF"
#define ARIES_WIFI_PASS            "Aries@123"
#define EXAMPLE_ESP_WIFI_CHANNEL   1
#define EXAMPLE_MAX_STA_CONN       1

static const char *TAG = "Aries_Line_Follower";

#define LED_PIN 2

int led_state = 0;

extern const uint8_t logo_start[] asm("_binary_Aries_png_start");
extern const uint8_t logo_end[]   asm("_binary_Aries_png_end");

char on_resp[] = "<!DOCTYPE html> <title> Aries </title><html> <head><fieldset ><center> <img src=\"Aries.png\"></head> <body> <meta name=\"viewport\"content=\"width=device-width, initial-scale=1\"><link rel=\"icon<style\"href=\"data:,\"> <style>body {text-align: center;font-family: \"Trebuchet MS\", Arial;margin-left:auto;margin-right:auto; }.slider {width: 300px; }</style> <style type=\"text/css\">html{  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #b30000; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>Line Follower</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\" ></head><body>  <h2>Line Follower</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i><p>Current state: <strong> M1</strong></p>        <p>          <a href=\"/M1on\"><button class=\"button\">M1</button></a>          <a href=\"/M2on\"><button class=\"button button2\">M2</button></a>  <a href=\"/STP\"><button class=\"button button2\">STOP</button></a>      </p>      </div>    </div>  </div></body></html>";
char off_resp[] = "<!DOCTYPE html> <title> Aries </title><html> <head><fieldset ><center> <img src=\"Aries.png\"></head> <body> <meta name=\"viewport\"content=\"width=device-width, initial-scale=1\"><link rel=\"icon<style\"href=\"data:,\"> <style>body {text-align: center;font-family: \"Trebuchet MS\", Arial;margin-left:auto;margin-right:auto; }.slider {width: 300px; }</style><style type=\"text/css\">html{  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #b30000; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>Line Follower</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\"></head><body>  <h2>Line Follower</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i> <p>Current state: <strong> M2</strong></p>        <p>          <a href=\"/M1on\"><button class=\"button\">M1</button></a>          <a href=\"/M2on\"><button class=\"button button2\">M2</button></a>  <a href=\"/STP\"><button class=\"button button2\">STOP</button></a>      </p>      </div>    </div>  </div></body></html>";
char stop_resp[] = "<!DOCTYPE html> <title> Aries </title><html> <head><fieldset ><center> <img src=\"Aries.png\"></head> <body> <meta name=\"viewport\"content=\"width=device-width, initial-scale=1\"><link rel=\"icon<style\"href=\"data:,\"> <style>body {text-align: center;font-family: \"Trebuchet MS\", Arial;margin-left:auto;margin-right:auto; }.slider {width: 300px; }</style><style type=\"text/css\">html{  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #b30000; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>Line Follower</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\"></head><body>  <h2>Line Follower</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i> <p>Current state: <strong> STOP</strong></p>        <p>          <a href=\"/M1on\"><button class=\"button\">M1</button></a>          <a href=\"/M2on\"><button class=\"button button2\">M2</button></a>  <a href=\"/STP\"><button class=\"button button2\">STOP</button></a>      </p>      </div>    </div>  </div></body></html>";


extern const uint8_t logo_start[] asm("_binary_Aries_png_start");
extern const uint8_t logo_end[]   asm("_binary_Aries_png_end");

esp_err_t on_png_handler(httpd_req_t *req)
{
	printf("!!! Sending on.png !!!\r\n");

	httpd_resp_set_type(req, "image/png");

	httpd_resp_send(req, (const char *)logo_start, (logo_end-1) - logo_start);

	return ESP_OK;
}

httpd_uri_t on_png = {
	.uri = "/Aries.png",
	.method = HTTP_GET,
	.handler = on_png_handler,
	/* Let's pass response string in user
	 * context to demonstrate it's usage */
	.user_ctx = NULL
};


static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ARIES_WIFI_SSID,
            .ssid_len = strlen(ARIES_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = ARIES_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                    .required = false,
            },
        },
    };
    if (strlen(ARIES_WIFI_PASS) == 0) {
        // wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ARIES_WIFI_SSID, ARIES_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

esp_err_t send_web_page(httpd_req_t *req)
{
    int response;
    if (led_state == 0)
    {
        response = httpd_resp_send(req, off_resp, HTTPD_RESP_USE_STRLEN);
       
    }
    else if(led_state == 1)
    {
        response = httpd_resp_send(req, on_resp, HTTPD_RESP_USE_STRLEN);
    }
    else
    {
        response = httpd_resp_send(req, stop_resp, HTTPD_RESP_USE_STRLEN);
    }
       
    return response;
}
esp_err_t get_req_handler(httpd_req_t *req)
{
    return send_web_page(req);
}

esp_err_t led_on_handler(httpd_req_t *req)
{
    gpio_set_level(LED_PIN, 1);
    led_state = 1;
    return send_web_page(req);
}

esp_err_t led_off_handler(httpd_req_t *req)
{
    gpio_set_level(LED_PIN, 0);
    led_state = 0;
    return send_web_page(req);
}

httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_req_handler,
    .user_ctx = NULL};

httpd_uri_t uri_on = {
    .uri = "/M1on",
    .method = HTTP_GET,
    .handler = led_on_handler,
    .user_ctx = NULL};

    httpd_uri_t uri_stop = {
    .uri = "/STP",
    .method = HTTP_GET,
    .handler = led_off_handler,
    .user_ctx = NULL};


httpd_uri_t uri_off = {
    .uri = "/M2on",
    .method = HTTP_GET,
    .handler = led_off_handler,
    .user_ctx = NULL};

httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &on_png);
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_on);
        httpd_register_uri_handler(server, &uri_off);
        httpd_register_uri_handler(server, &uri_stop);
       
    }

    return server;
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "WIFI_MODE_AP");
    wifi_init_softap();
    //gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    led_state = 0;
    ESP_LOGI(TAG, "LF Web Server is running ... ...\n");
    setup_server();
}
