/*  Author: Saiteja Vellanki.
    Project: Industrial_Line_Follower->Proto
    Programming Lang: Embedded C
    Controller: ESP32 
    Wireless: WiFi Access point
    control: Local Web server
    client: Aries solutions pvt ltd
    A Finite State Machine (FSM) Approach
    code access: https://github.com/saitez/Aries_Line_Follower
    Binary Path: Aries_Line_Follower/build/Aries_LF.bin
    ELF Path: Aries_Line_Follower/build/Aries_LF.elf
    Date:31-10-2023
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
#include "esp_app_desc.h"
#include <inttypes.h> 
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

/*Macro define's wifi SSID*/ 
#define ARIES_WIFI_SSID            "Aries_LF"
/*Macro define's wifi Pass*/ 
#define ARIES_WIFI_PASS            "Aries@123"
/*Macro define's wifi channel connection*/ 
#define ARIES_ESP_WIFI_CHANNEL   1
/*Macro define's wifi no. of stations can connect*/ 
#define ARIES_MAX_STA_CONN       1

static const char *TAG = "Aries_Line_Follower";

/*Macro's define's PWM pins to control motors*/ 
#define ARIES_PWM_MOTOR_M1_PIN  25
#define ARIES_PWM_MOTOR_M2_PIN  26

/*Macro's define's I/O Digital pins for IR sensors*/ 
#define ARIES_IR_SENSOR_PIN_1  18
#define ARIES_IR_SENSOR_PIN_2  19
#define ARIES_IR_SENSOR_PIN_3  21
#define ARIES_IR_SENSOR_PIN_4  22
#define ARIES_IR_SENSOR_PIN_5  23

/* Control Macro's */ 
#define LED_TEST 1
//#define PROJ_DET 1
#define DEBUG_LEVEL_xx1 1
#define DEBUG_LEVEL_xx2 1
#define DEBUG_LEVEL_xx3 1

/*Macro define's for test LED*/ 
#ifdef  LED_TEST
#define LED_PIN 2
#endif

/*Macro's define for test IR states*/ 
#define IR_STATE_HIGH 1
#define IR_STATE_LOW  0

/*Macro's define for Button states*/ 
#define BUTTON_STATE_HIGH 1
#define BUTTON_STATE_LOW  0


/*User-defined enum control Error states*/
typedef enum Error_States
{
    FAILURE =0x00,
    SUCCESS
}error_st;

/*User-defined enum control finite state_machine motor commands*/
typedef enum Motor_CMD
{
    STOP =0x00,
    START
}motor_cmd;

/*User-defined enum finite state_machine motor states*/
typedef enum Motor_States
{
   MOTOR_OFF =0x00,
   MOTOR_ON

}motor_states;

/*User-defined enum finite state_machine machine type*/
typedef enum Machine_Type
{
   MACHINE_1 =0x01,
   MACHINE_2,
   MACHINE_3,
   MACHINE_4
}mach_typ;

/*User-defined enum finite state_machine motor direction*/
typedef enum Motor_Direction
{
   FORWARD =0x01,
   BACKWARD,
   LEFT,
   RIGHT
}motor_dir;

/*User-defined enum finite state_machine for IR_Sensor selecion*/
typedef enum IR_Sensor
{
   IR_SENSOR_1 =0x01,
   IR_SENSOR_2,
   IR_SENSOR_3,
   IR_SENSOR_4,
   IR_SENSOR_5
}ir_sens;


// /*User-defined structure to hold sensor global data*/
// typedef struct Sensor_data
// {
//     uint8_t sens_1:1;
//     uint8_t sens_2:1;
//     uint8_t sens_3:1;
//     uint8_t sens_4:1;
//     uint8_t sens_5:1;
//     uint8_t reserved:3;

// }s_data;

// /*User-defined structure to hold global flags*/
// typedef struct Global_flag
// {
//     uint8_t gf_1:1;
//     uint8_t gf_2:1;
//     uint8_t gf_3:1;
//     uint8_t gf_4:1;
//     uint8_t gf_5:1;
//     uint8_t reserved:3;

// }g_flag;

/*Global variables*/
//g_flag *global_flag;
//s_data *sensor_data;
/*Sensor output global variable*/
uint8_t sens_1,sens_2,sens_3,sens_4,sens_5;

/*Machine selection global flag*/
uint8_t gf_1,gf_2,gf_3,gf_4,gf_5;

/*Complete project details Name, version etc*/
const esp_app_desc_t *app_desc = NULL;

/*Function proto-types for FSM control*/
error_st Motor_cmd(motor_cmd cmd);
error_st Motor_state(motor_states states);
error_st Machine_type(mach_typ type);
error_st Motor_Dir_update(mach_typ type, motor_dir direction);
error_st Ir_Sens_selection(ir_sens selection);

/*Function proto-types for Motor control*/
static void motor_control_1(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle);
static void motor_control_2(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle);
static void manual_motor_stop(void);
static void auto_motor_stop(void);


/*char array's  define's HTML code to ON/OFF channels M1, M2, M3, M4 and STOP*/ 
char M1_resp[] = "<!DOCTYPE html> <title> Aries </title><html> <head><fieldset ><center> <img src=\"Aries.png\"></head> <body> <meta name=\"viewport\"content=\"width=device-width, initial-scale=1\"><link rel=\"icon<style\"href=\"data:,\"> <style>body {text-align: center;font-family: \"Trebuchet MS\", Arial;margin-left:auto;margin-right:auto; }.slider {width: 300px; }</style><style type=\"text/css\">html{  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #364cf4; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.button3 {  background-color: #364cf4; //blue color}.button4 {  background-color: #364cf4; //blue color}.button5 {  background-color: #ff0000; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>Line Follower</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\"></head><body>  <h2>Line Follower</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i> <p>Current state: <strong> M1</strong></p>        <p>          <a href=\"/M1on\"><button class=\"button\">M1</button></a>          <a href=\"/M2on\"><button class=\"button button2\">M2</button></a> <a href=\"/M3on\"><button class=\"button button3\">M3</button></a> <a href=\"/M4on\"><button class=\"button button4\">M4</button></a> <a href=\"/STP\"><button class=\"button button5\">STOP</button></a>      </p>      </div>    </div>  </div></body></html>";
char M2_resp[] = "<!DOCTYPE html> <title> Aries </title><html> <head><fieldset ><center> <img src=\"Aries.png\"></head> <body> <meta name=\"viewport\"content=\"width=device-width, initial-scale=1\"><link rel=\"icon<style\"href=\"data:,\"> <style>body {text-align: center;font-family: \"Trebuchet MS\", Arial;margin-left:auto;margin-right:auto; }.slider {width: 300px; }</style><style type=\"text/css\">html{  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #364cf4; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.button3 {  background-color: #364cf4; //blue color}.button4 {  background-color: #364cf4; //blue color}.button5 {  background-color: #ff0000; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>Line Follower</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\"></head><body>  <h2>Line Follower</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i> <p>Current state: <strong> M2</strong></p>        <p>          <a href=\"/M1on\"><button class=\"button\">M1</button></a>          <a href=\"/M2on\"><button class=\"button button2\">M2</button></a> <a href=\"/M3on\"><button class=\"button button3\">M3</button></a> <a href=\"/M4on\"><button class=\"button button4\">M4</button></a> <a href=\"/STP\"><button class=\"button button5\">STOP</button></a>      </p>      </div>    </div>  </div></body></html>";
char M3_resp[] = "<!DOCTYPE html> <title> Aries </title><html> <head><fieldset ><center> <img src=\"Aries.png\"></head> <body> <meta name=\"viewport\"content=\"width=device-width, initial-scale=1\"><link rel=\"icon<style\"href=\"data:,\"> <style>body {text-align: center;font-family: \"Trebuchet MS\", Arial;margin-left:auto;margin-right:auto; }.slider {width: 300px; }</style><style type=\"text/css\">html{  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #364cf4; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.button3 {  background-color: #364cf4; //blue color}.button4 {  background-color: #364cf4; //blue color}.button5 {  background-color: #ff0000; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>Line Follower</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\"></head><body>  <h2>Line Follower</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i> <p>Current state: <strong> M3</strong></p>        <p>          <a href=\"/M1on\"><button class=\"button\">M1</button></a>          <a href=\"/M2on\"><button class=\"button button2\">M2</button></a> <a href=\"/M3on\"><button class=\"button button3\">M3</button></a> <a href=\"/M4on\"><button class=\"button button4\">M4</button></a> <a href=\"/STP\"><button class=\"button button5\">STOP</button></a>      </p>      </div>    </div>  </div></body></html>";
char M4_resp[] = "<!DOCTYPE html> <title> Aries </title><html> <head><fieldset ><center> <img src=\"Aries.png\"></head> <body> <meta name=\"viewport\"content=\"width=device-width, initial-scale=1\"><link rel=\"icon<style\"href=\"data:,\"> <style>body {text-align: center;font-family: \"Trebuchet MS\", Arial;margin-left:auto;margin-right:auto; }.slider {width: 300px; }</style><style type=\"text/css\">html{  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #364cf4; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.button3 {  background-color: #364cf4; //blue color}.button4 {  background-color: #364cf4; //blue color}.button5 {  background-color: #ff0000; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>Line Follower</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\"></head><body>  <h2>Line Follower</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i> <p>Current state: <strong> M4</strong></p>        <p>          <a href=\"/M1on\"><button class=\"button\">M1</button></a>          <a href=\"/M2on\"><button class=\"button button2\">M2</button></a> <a href=\"/M3on\"><button class=\"button button3\">M3</button></a> <a href=\"/M4on\"><button class=\"button button4\">M4</button></a> <a href=\"/STP\"><button class=\"button button5\">STOP</button></a>      </p>      </div>    </div>  </div></body></html>";

char stop_resp[] = "<!DOCTYPE html> <title> Aries </title><html> <head><fieldset ><center> <img src=\"Aries.png\"></head> <body> <meta name=\"viewport\"content=\"width=device-width, initial-scale=1\"><link rel=\"icon<style\"href=\"data:,\"> <style>body {text-align: center;font-family: \"Trebuchet MS\", Arial;margin-left:auto;margin-right:auto; }.slider {width: 300px; }</style><style type=\"text/css\">html{  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #364cf4; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.button3 {  background-color: #364cf4; //blue color}.button4 {  background-color: #364cf4; //blue color}.button5 {  background-color: #ff0000; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>Line Follower</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\"></head><body>  <h2>Line Follower</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i> <p>Current state: <strong> STOP</strong></p>    <p>          <a href=\"/M1on\"><button class=\"button\">M1</button></a>          <a href=\"/M2on\"><button class=\"button button2\">M2</button></a> <a href=\"/M3on\"><button class=\"button button3\">M3</button></a> <a href=\"/M4on\"><button class=\"button button4\">M4</button></a> <a href=\"/STP\"><button class=\"button button5\">STOP</button></a>      </p>      </div>    </div>  </div></body></html>";

/*char array's  define's extracting Company logo from build binary*/ 
extern const uint8_t logo_start[] asm("_binary_Aries_png_start");
extern const uint8_t logo_end[]   asm("_binary_Aries_png_end");

/*function handles http response for logo injection*/ 
esp_err_t on_png_handler(httpd_req_t *req)
{
	printf("!!! Sending on.png !!!\r\n");

	httpd_resp_set_type(req, "image/png");

	httpd_resp_send(req, (const char *)logo_start, (logo_end-1) - logo_start);

	return ESP_OK;
}

/*structure handles http get for logo injection*/ 
httpd_uri_t on_png = {
	.uri = "/Aries.png",
	.method = HTTP_GET,
	.handler = on_png_handler,
	.user_ctx = NULL
};


static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
#ifdef DEBUG_LEVEL_xx1
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                MAC2STR(event->mac), event->aid);
#endif

        
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
#ifdef DEBUG_LEVEL_xx1
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
                MAC2STR(event->mac), event->aid);
#endif
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
            .channel = ARIES_ESP_WIFI_CHANNEL,
            .password = ARIES_WIFI_PASS,
            .max_connection = ARIES_MAX_STA_CONN,
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
#ifdef DEBUG_LEVEL_xx1
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ARIES_WIFI_SSID, ARIES_WIFI_PASS, ARIES_ESP_WIFI_CHANNEL);
#endif
}



/*Brief function handles IR sensor selection*/
/*function arguments IR_SENSOR_1/2/3/4/5 */
/*error_st Ir_Sens_selection(ir_sens selection) returns SUCCESS/FAILURE */
error_st Ir_Sens_selection(ir_sens selection)
{
    uint8_t ir_sens=selection;
    uint8_t err_code;
    switch (ir_sens)
    {
        case IR_SENSOR_1:
        sens_1 = gpio_get_level(ARIES_IR_SENSOR_PIN_1);
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "IR_SENSOR_1 :%d",sens_1);
#endif
        err_code = SUCCESS;
        break;
        case IR_SENSOR_2:
        sens_2 = gpio_get_level(ARIES_IR_SENSOR_PIN_2);
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "IR_SENSOR_2 :%d",sens_2);
#endif
        err_code = SUCCESS;
        break;
        case IR_SENSOR_3:
        sens_3 = gpio_get_level(ARIES_IR_SENSOR_PIN_3);
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "IR_SENSOR_3 :%d",sens_3);
#endif
        err_code = SUCCESS;
        break;
        case IR_SENSOR_4:
        sens_4 = gpio_get_level(ARIES_IR_SENSOR_PIN_4);
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "IR_SENSOR_4 :%d",sens_4);
#endif
        err_code = SUCCESS;
        break;
        case IR_SENSOR_5:
        sens_5 = gpio_get_level(ARIES_IR_SENSOR_PIN_5);
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "IR_SENSOR_5 :%d",sens_5);
#endif
        err_code = SUCCESS;
        break;

        default:
        err_code = FAILURE;
#ifdef DEBUG_LEVEL_xx3
        ESP_LOGI(TAG, "No sensor selected");
#endif
        break;
    }
    return err_code;

}


/*Brief function handles machine type*/
/*function arguments MACHINE_1/2/3/4 */
/*error_st Machine_type(mach_typ type) returns SUCCESS/FAILURE */
error_st Machine_type(mach_typ type)
{
    uint8_t mach_typ=type;
    uint8_t err_code;

    switch (mach_typ)
    {
        case MACHINE_1:
        Motor_cmd(START);
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "M1-ON SWITCH CASE");
#endif
        gf_1 = BUTTON_STATE_HIGH;
        Motor_state(MOTOR_ON);
        gf_2 = BUTTON_STATE_LOW;
        gf_3 = BUTTON_STATE_LOW;
        gf_4 = BUTTON_STATE_LOW;
        gf_5 = BUTTON_STATE_LOW;
        err_code = SUCCESS;
        break;
        case MACHINE_2:
        Motor_cmd(START);
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "M2-ON SWITCH CASE");
#endif
        gf_1 = BUTTON_STATE_LOW;
        gf_2 = BUTTON_STATE_HIGH;
        Motor_state(MOTOR_ON);
        gf_3 = BUTTON_STATE_LOW;
        gf_4 = BUTTON_STATE_LOW;
        gf_5 = BUTTON_STATE_LOW;
        err_code = SUCCESS;
        break;
        case MACHINE_3:
        Motor_cmd(START);
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "M3-ON SWITCH CASE");
#endif
        gf_1 = BUTTON_STATE_LOW;
        gf_2 = BUTTON_STATE_LOW;
        gf_3 = BUTTON_STATE_HIGH;
        Motor_state(MOTOR_ON);
        gf_4 = BUTTON_STATE_LOW;
        gf_5 = BUTTON_STATE_LOW;
        err_code = SUCCESS;
        break;
        case MACHINE_4:
        Motor_cmd(START);
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "M4-ON SWITCH CASE");
#endif
        gf_1 = BUTTON_STATE_LOW;
        gf_2 = BUTTON_STATE_LOW;
        gf_3 = BUTTON_STATE_LOW;
        gf_4 = BUTTON_STATE_HIGH;
        gf_5 = BUTTON_STATE_LOW;
        Motor_state(MOTOR_ON);
        err_code = SUCCESS;
        break;
        
        default:
        Motor_state(MOTOR_OFF);
        err_code = FAILURE;
#ifdef DEBUG_LEVEL_xx3
        ESP_LOGI(TAG, "FAILURE");
#endif
        break;
           
    }
    return err_code;
   
}

/*Brief function handles motor command*/
/*function arguments START/STOP */
/*error_st Motor_cmd(motor_cmd cmd) returns SUCCESS/FAILURE */
error_st Motor_cmd(motor_cmd cmd)
{
    uint8_t motor_cmd=cmd;
    uint8_t err_code;
    switch (motor_cmd)
    {
        case STOP:
        gf_1 = 0;
        gf_2 = 0;
        gf_3 = 0;
        gf_4 = 0;
        gf_5 = BUTTON_STATE_HIGH;
        err_code = SUCCESS;
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "MOTOR STOP");
#endif
        break;
        case START:
        err_code = SUCCESS;
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "MOTOR START");
#endif
        break;
        
        default:
        err_code = FAILURE;
#ifdef DEBUG_LEVEL_xx3
        ESP_LOGI(TAG, "FAILURE");
#endif
        break;
           
    }
    return err_code;
   
}

/*Brief function handles Motor_state*/
/*function argumentsMOTOR_OFF/MOTOR_ON*/
/*error_st Motor_state(motor_states states) returns SUCCESS/FAILURE */
error_st Motor_state(motor_states states)
{
    uint8_t motor_state=states;
    uint8_t err_code;
    switch (motor_state)
    {
        case MOTOR_OFF:
        auto_motor_stop();
        err_code = SUCCESS;
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "AUTO MOTOR OFF");
#endif
        break;
        case MOTOR_ON:
        err_code = SUCCESS;
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "MOTOR ON");
#endif
        break;
        
        default:
        err_code = FAILURE;
#ifdef DEBUG_LEVEL_xx3
        ESP_LOGI(TAG, "FAILURE");
#endif
        break;
           
    }
    return err_code;
   
}

error_st Motor_Dir_update(mach_typ type, motor_dir direction)
{
    uint8_t motor_type=type;
    uint8_t motor_dir=direction;
    uint8_t err_code;

    switch (direction)
    {
        case FORWARD:
        err_code = SUCCESS;
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "FORWARD");
#endif
        break;
        case BACKWARD:
        err_code = SUCCESS;
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "BACKWARD");
#endif
        break;
        case LEFT:
        err_code = SUCCESS;
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "LEFT");
#endif
        break;
        case RIGHT:
        err_code = SUCCESS;
#ifdef DEBUG_LEVEL_xx2
        ESP_LOGI(TAG, "RIGHT");
#endif
        break;
        
        default:
        err_code = FAILURE;
#ifdef DEBUG_LEVEL_xx3
        ESP_LOGI(TAG, "FAILURE");
#endif
        break;
           
    }
    return err_code;

}

static void manual_motor_stop(void)
{
    motor_control_1(MCPWM_UNIT_0, MCPWM_TIMER_0, 0);
    motor_control_2(MCPWM_UNIT_1, MCPWM_TIMER_1, 0);
}

static void auto_motor_stop()
{
    if((sens_1 == IR_STATE_HIGH) && (sens_2 == IR_STATE_HIGH))
    {
        manual_motor_stop();
    }
    else if((sens_2 == IR_STATE_HIGH) && (sens_3 == IR_STATE_HIGH))
    {
        manual_motor_stop();
    }
    else if((sens_3 == IR_STATE_HIGH) && (sens_4 == IR_STATE_HIGH))
    {
        manual_motor_stop();
    }
    else if((sens_4 == IR_STATE_HIGH) && (sens_5 == IR_STATE_HIGH))
    {
        manual_motor_stop();
    }
    else
    {
#ifdef DEBUG_LEVEL_xx3
        ESP_LOGI(TAG, "Auto STOP Failed");
#endif
    }

}
static void motor_control_1(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}

static void motor_control_2(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_B, duty_cycle);
    mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
}
    

esp_err_t m1_send_web_page(httpd_req_t *req)
{
    int response;
    response = httpd_resp_send(req, M1_resp, HTTPD_RESP_USE_STRLEN);
    
    return response;
}

esp_err_t m2_send_web_page(httpd_req_t *req)
{
    int response;
    response = httpd_resp_send(req, M2_resp, HTTPD_RESP_USE_STRLEN);
    
    return response;
}
esp_err_t m3_send_web_page(httpd_req_t *req)
{
    int response;
    response = httpd_resp_send(req, M3_resp, HTTPD_RESP_USE_STRLEN);
    
    return response;
}
esp_err_t m4_send_web_page(httpd_req_t *req)
{
    int response;
    response = httpd_resp_send(req, M4_resp, HTTPD_RESP_USE_STRLEN);
    
    return response;
}
esp_err_t stop_send_web_page(httpd_req_t *req)
{
    int response;
    response = httpd_resp_send(req, stop_resp, HTTPD_RESP_USE_STRLEN);
    
    return response;
}
esp_err_t get_req_handler(httpd_req_t *req)
{
    return stop_send_web_page(req);
}

/*Actual function handlers to control m1, m2, m3, m4, stop commands*/ 
esp_err_t m1_on_handler(httpd_req_t *req)
{
#ifdef DEBUG_LEVEL_xx1
    ESP_LOGI(TAG, "M1-ON");
#endif
    if(Machine_type(MACHINE_1)!=SUCCESS)
    {
#ifdef DEBUG_LEVEL_xx3
        ESP_LOGI(TAG, "M1 Failed to ON");
#endif
    }
   
    
    return m1_send_web_page(req);
}

esp_err_t m2_on_handler(httpd_req_t *req)
{
#ifdef DEBUG_LEVEL_xx1
    ESP_LOGI(TAG, "M2-ON");
#endif
    if(Machine_type(MACHINE_2)!=SUCCESS)
    {
#ifdef DEBUG_LEVEL_xx3
        ESP_LOGI(TAG, "M2 Failed to ON");
#endif
    }
   
    
    return m2_send_web_page(req);
}

esp_err_t m3_on_handler(httpd_req_t *req)
{
#ifdef DEBUG_LEVEL_xx1
    ESP_LOGI(TAG, "M3-ON");
#endif
    if(Machine_type(MACHINE_3)!=SUCCESS)
    {
#ifdef DEBUG_LEVEL_xx3
        ESP_LOGI(TAG, "M3 Failed to ON");
#endif
    }
   
    
    return m3_send_web_page(req);
}

esp_err_t m4_on_handler(httpd_req_t *req)
{
#ifdef DEBUG_LEVEL_xx1
    ESP_LOGI(TAG, "M4-ON");
#endif
    if(Machine_type(MACHINE_4)!=SUCCESS)
    {
#ifdef DEBUG_LEVEL_xx3
        ESP_LOGI(TAG, "M4 Failed to ON");
#endif
    }
     
    return m4_send_web_page(req);
}

esp_err_t stop_handler(httpd_req_t *req)
{
#ifdef DEBUG_LEVEL_xx1
    ESP_LOGI(TAG, "STOP");
#endif
    if(Motor_cmd(STOP)!=SUCCESS)
    {
#ifdef DEBUG_LEVEL_xx3
        ESP_LOGI(TAG, "STOP command failed");
#endif
    }
    
    return stop_send_web_page(req);
}

httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_req_handler,
    .user_ctx = NULL};

httpd_uri_t m1_on = {
    .uri = "/M1on",
    .method = HTTP_GET,
    .handler = m1_on_handler,
    .user_ctx = NULL};

    httpd_uri_t uri_stop = {
    .uri = "/STP",
    .method = HTTP_GET,
    .handler = stop_handler,
    .user_ctx = NULL};


httpd_uri_t m2_on = {
    .uri = "/M2on",
    .method = HTTP_GET,
    .handler = m2_on_handler,
    .user_ctx = NULL};

httpd_uri_t m3_on = {
    .uri = "/M3on",
    .method = HTTP_GET,
    .handler = m3_on_handler,
    .user_ctx = NULL};

httpd_uri_t m4_on = {
    .uri = "/M4on",
    .method = HTTP_GET,
    .handler = m4_on_handler,
    .user_ctx = NULL};


httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &on_png);
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &m1_on);
        httpd_register_uri_handler(server, &m2_on);
        httpd_register_uri_handler(server, &m3_on);
        httpd_register_uri_handler(server, &m4_on);
        httpd_register_uri_handler(server, &uri_stop);
       
    }

    return server;
}

/*Brief function handles test LED init*/
/*function arguments NONE */
/*void Led_test_init() returns NONE */
void Led_test_init()
{
    esp_rom_gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    ESP_LOGI(TAG, "Test Led init success\n");
}

/*Brief function handles test IR sensor init*/
/*function arguments NONE */
/*void IR_sensor_init() returns NONE */
void IR_sensor_init()
{
    esp_rom_gpio_pad_select_gpio(ARIES_IR_SENSOR_PIN_1);
    gpio_set_direction(ARIES_IR_SENSOR_PIN_1, GPIO_MODE_INPUT);
    esp_rom_gpio_pad_select_gpio(ARIES_IR_SENSOR_PIN_2);
    gpio_set_direction(ARIES_IR_SENSOR_PIN_2, GPIO_MODE_INPUT);
    esp_rom_gpio_pad_select_gpio(ARIES_IR_SENSOR_PIN_3);
    gpio_set_direction(ARIES_IR_SENSOR_PIN_3, GPIO_MODE_INPUT);
    esp_rom_gpio_pad_select_gpio(ARIES_IR_SENSOR_PIN_4);
    gpio_set_direction(ARIES_IR_SENSOR_PIN_4, GPIO_MODE_INPUT);
    esp_rom_gpio_pad_select_gpio(ARIES_IR_SENSOR_PIN_5);
    gpio_set_direction(ARIES_IR_SENSOR_PIN_5, GPIO_MODE_INPUT);
#ifdef DEBUG_LEVEL_xx3
    ESP_LOGI(TAG, "All sensor pins init success\n");
#endif
}

void Machine_processing_task(void *pvParameter)
{
    for(;;)
    {
        if(gf_1 == BUTTON_STATE_HIGH)
        {
            /*Processing Target-> Machine-1*/
            Ir_Sens_selection(IR_SENSOR_1);
            Ir_Sens_selection(IR_SENSOR_2);
           
        }
        else if(gf_2 == BUTTON_STATE_HIGH)
        {
            /*Processing Target-> Machine-2*/
            Ir_Sens_selection(IR_SENSOR_2);
            Ir_Sens_selection(IR_SENSOR_3);
        }
        else if(gf_3 == BUTTON_STATE_HIGH)
        {
            /*Processing Target-> Machine-3*/
            Ir_Sens_selection(IR_SENSOR_3);
            Ir_Sens_selection(IR_SENSOR_4);
        }
        else if(gf_4 == BUTTON_STATE_HIGH)
        {
            /*Processing Target-> Machine-4*/
            Ir_Sens_selection(IR_SENSOR_4);
            Ir_Sens_selection(IR_SENSOR_5);
        }
        else if(gf_5 == BUTTON_STATE_HIGH)
        {
            manual_motor_stop();
        }
        else
        {
#ifdef DEBUG_LEVEL_xx1
            ESP_LOGI(TAG, "No Sensor selected!!!");
#endif
        }
        Motor_state(MOTOR_OFF);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void app_proj_details()
{
    app_desc = esp_app_get_description();
    ESP_EARLY_LOGI(TAG, "Project name:     %s", app_desc->project_name);
    ESP_EARLY_LOGI(TAG, "App version:      %s", app_desc->version);
    ESP_EARLY_LOGI(TAG, "Secure version:   %d", app_desc->secure_version);
    ESP_EARLY_LOGI(TAG, "Compile time:     %s %s", app_desc->date, app_desc->time);
}

static void mcpwm_gpio_initialize(void)
{
#ifdef DEBUG_LEVEL_xx3
    printf("Aries initializing mcpwm gpio...\n");
#endif
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, ARIES_PWM_MOTOR_M1_PIN);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, ARIES_PWM_MOTOR_M2_PIN);
}

void mcpwm_control(void)
{
  
    //1. mcpwm gpio initialization
    mcpwm_gpio_initialize();

    //2. initial mcpwm configuration
#ifdef DEBUG_LEVEL_xx3
    printf("Configuring Initial Parameters of mcpwm...\n");
#endif
    mcpwm_config_t pwm_config;
    pwm_config.frequency =18000;   //frequency = 18kHz as per the Driver,
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode =MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_1, &pwm_config);    
    
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
#ifdef  PROJ_DET
    app_proj_details();
#endif

#ifdef DEBUG_LEVEL_xx3
    ESP_LOGI(TAG, "WIFI_MODE_AP");
#endif
    wifi_init_softap();

#ifdef LED_TEST
    Led_test_init();
#endif

    IR_sensor_init();
#ifdef DEBUG_LEVEL_xx3
    ESP_LOGI(TAG, "LF Web Server is running ... ...\n");
#endif
    setup_server();
    mcpwm_control();
    xTaskCreate(&Machine_processing_task, "task_run", 1024 * 5, NULL, 3 , NULL); 
}
