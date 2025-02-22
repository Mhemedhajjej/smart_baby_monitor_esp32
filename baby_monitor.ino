#include "esp_camera.h"
#include <WiFi.h>
#include "freertos/event_groups.h"
#include "DHT.h"
#include "config.h"
#include "camera_pins.h"
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <ESP_Mail_Client.h> // Include the ESP32 Mail Client library

/******************************************************************* 
******* SYSTEM CONFIGURATIONS ************************************** 
*******************************************************************/

// Enter your WiFi credentials
const char* ssid = "SET_YOUR_WIFI_SSID_HERE";
const char* password = "SET_YOUR_WIFI_PASSWORD_HERE";

// Enter your whatsapp contact info
const String phoneNumber = "SET_YOUR_WHATSAPP_NUMBER_HERE";
const String apiKey = "SET_YOUR_API_KEY_HERE"; //got from callmebot.com


// Email configuration
const char* smtp_server = "smtp.gmail.com";
const int smtp_port = 465;
const char* email_sender_account = "SET_YOUR_SENDER_EMAIL_HERE";
const char* email_sender_password = "SET_YOUR_SENDER_EMAIL_APP_PASSWORD";
const char* email_recipient = "SET_YOUR_RECIPIENT_EMAIL_HERE";

/******************************************************************* 
******* STATIC FUNCTIONS ******************************************* 
*******************************************************************/

static void startCameraServer();
static void setupLedFlash(int pin);
static void esp32_cam_get_config(camera_config_t *config);
static void esp32_cam_get_config(camera_config_t *config);

static esp_err_t setup_camera();
static esp_err_t setup_camera_sensor();
static void setup_external_sensors();
static void temperature_sensor_setup();

static String check_room_conditions(void);

static esp_err_t alert_open_na(void);
static esp_err_t send_alert_notification(String msg);
static esp_err_t print_alert_notification(String msg);
static esp_err_t send_alert_via_whatsapp(String msg);
static esp_err_t send_alert_via_email(String msg);

static void IRAM_ATTR read_temp_hum_isr();
static void IRAM_ATTR sound_monitor_isr();

/******************************************************************* 
******* GLOBALS **************************************************** 
*******************************************************************/

hw_timer_t *g_temp_hum_read_timer = NULL;
EventGroupHandle_t g_event_group;
EventBits_t g_event_bits;

DHT dht(DHTPIN, DHTTYPE);

String status;

struct alert_ops_s {
        esp_err_t (*open)(void);
        esp_err_t (*send)(String message);
} g_alert_ops[ALERT_METHOD_MAX] = {
        [ALERT_METHOD_EMAIL] = {
                .open = alert_open_na,
                .send = send_alert_via_email
        },
        [ALERT_METHOD_WHATSAPP] = {
                .open = alert_open_na,
                .send = send_alert_via_whatsapp,
        },
        [ALERT_METHOD_BLYNK_IOT] = {
                .open = NULL,
                .send = NULL
        },
        [ALERT_METHOD_LOG] = {
                .open = alert_open_na,
                .send = print_alert_notification,
        }
};

struct alert_ops_s *alert = NULL;
static uint8_t g_alert_driver = ALERT_METHOD_EMAIL;

/******************************************************************* 
******* EXTERN FUNCTIONS ******************************************* 
*******************************************************************/

extern esp_err_t send_ws_message(const char *message);

/******************************************************************* 
******* FUNCTIONS ************************************************** 
*******************************************************************/

void setup() {
        esp_err_t err;

        Serial.begin(115200);
        Serial.setDebugOutput(true);
        Serial.println();

        /* start camera */
        err = setup_camera();
        if (err == ESP_OK)
                err = setup_camera_sensor();

        if (err == ESP_OK) {
                Serial.println("Camera sensor started");
                /* start wifi interface */
                WiFi.begin(ssid, password);
                WiFi.setSleep(false);
                while (WiFi.status() != WL_CONNECTED) {
                        delay(500);
                        Serial.print(".");
                }
                Serial.println("");
                Serial.println("WiFi connected");
        }

        /* start camera web server */
        if (err == ESP_OK) {
                startCameraServer();
                Serial.print("Camera Server Ready! Use 'http://");
                Serial.print(WiFi.localIP());
                Serial.println("' to connect");
        }
        
        /* create event group for the exteral events of the monitor/alarm system */
        if (err == ESP_OK) {
              g_event_group = xEventGroupCreate();
              configASSERT(g_event_group);
        }

        /* setup ecternal sensors (sound, temperature, humidity, etc..)*/
        if (err == ESP_OK)
                setup_external_sensors();
        
        /* set alert notifier method */
        if (err == ESP_OK) {
                alert = &g_alert_ops[g_alert_driver];
                alert->open();
        }
}

void loop() 
{
        /* collect sensor data and decide what to do */
        g_event_bits = xEventGroupWaitBits(
                g_event_group,   /* The event group being tested. */
                BIT0 | BIT1 | BIT2, /* The bits within the event group to wait for. */
                pdTRUE,        /* BITs should be cleared before returning. */
                pdFALSE,       /* Don't wait for both bits, either bit will do. */
                portMAX_DELAY);/* block */

        if (g_event_bits & got_temp) {
                /* time point to check room conditions (temp + humd)*/
                status = check_room_conditions();

                /* any limit violation -> send alert to user(s) */
                if (status != "room conditions ok")
                        send_alert_notification(status);
        } else  if (g_event_bits & got_sound) {
                /* baby crying -> sound monitor disabled to avoid continous interrupt trigger -> send alert to user(s)*/
                send_alert_notification("Status: AWAKE \n");
        } else if (g_event_bits & got_baby_happy) {
                /* user notified via baby button in browser that baby is cared -> re-enable sound sensor */
                Serial.println("mon left the range & baby is happy");
                sound_detector_enable();
                send_ws_message("normal"); // websocket to reflect normal status in baby button in browser
        }
}

/* function to pass event group handle to other files (app_httpd)*/
EventGroupHandle_t *get_event_group(void)
{
    return &g_event_group;
}

/*******************************************************************************
******** Setup helper function *************************************************
********************************************************************************/

static void esp32_cam_get_config(camera_config_t *config)
{
        config->ledc_channel = LEDC_CHANNEL_0;
        config->ledc_timer = LEDC_TIMER_0;
        config->pin_d0 = Y2_GPIO_NUM;
        config->pin_d1 = Y3_GPIO_NUM;
        config->pin_d2 = Y4_GPIO_NUM;
        config->pin_d3 = Y5_GPIO_NUM;
        config->pin_d4 = Y6_GPIO_NUM;
        config->pin_d5 = Y7_GPIO_NUM;
        config->pin_d6 = Y8_GPIO_NUM;
        config->pin_d7 = Y9_GPIO_NUM;
        config->pin_xclk = XCLK_GPIO_NUM;
        config->pin_pclk = PCLK_GPIO_NUM;
        config->pin_vsync = VSYNC_GPIO_NUM;
        config->pin_href = HREF_GPIO_NUM;
        config->pin_sccb_sda = SIOD_GPIO_NUM;
        config->pin_sccb_scl = SIOC_GPIO_NUM;
        config->pin_pwdn = PWDN_GPIO_NUM;
        config->pin_reset = RESET_GPIO_NUM;
        config->xclk_freq_hz = 20000000;
        config->frame_size = FRAMESIZE_UXGA;
        config->pixel_format = PIXFORMAT_JPEG; // for streaming
        config->pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
        config->grab_mode = CAMERA_GRAB_WHEN_EMPTY;
        config->fb_location = CAMERA_FB_IN_PSRAM;
        config->jpeg_quality = 12;
        config->fb_count = 1;
  
        /* if PSRAM IC present, init with UXGA resolution and higher JPEG quality
        *  for larger pre-allocated frame buffer.
        */
        if (config->pixel_format == PIXFORMAT_JPEG) {
                if (psramFound()) {
                        config->jpeg_quality = 10;
                        config->fb_count = 2;
                        config->grab_mode = CAMERA_GRAB_LATEST;
                } else {
                        // Limit the frame size when PSRAM is not available
                        config->frame_size = FRAMESIZE_SVGA;
                        config->fb_location = CAMERA_FB_IN_DRAM;
                }
        } else {
                // Best option for face detection/recognition
                config->frame_size = FRAMESIZE_240X240;
                #if CONFIG_IDF_TARGET_ESP32S3
                config->fb_count = 2;
                #endif
        }

        #if defined(CAMERA_MODEL_ESP_EYE)
        pinMode(13, INPUT_PULLUP);
        pinMode(14, INPUT_PULLUP);
        #endif
}

static esp_err_t setup_camera()
{
        camera_config_t cam_config;

        /* start camera */
        esp32_cam_get_config(&cam_config);
        return esp_camera_init(&cam_config);
}

static esp_err_t setup_camera_sensor()
{
        sensor_t *cam_sensor = esp_camera_sensor_get();
        // initial sensors are flipped vertically and colors are a bit saturated
        if (cam_sensor->id.PID == OV3660_PID) {
                cam_sensor->set_vflip(cam_sensor, 1); // flip it back
                cam_sensor->set_brightness(cam_sensor, 1); // up the brightness just a bit
                cam_sensor->set_saturation(cam_sensor, -2); // lower the saturation
        }
        #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
        cam_sensor->set_vflip(cam_sensor, 1);
        cam_sensor->set_hmirror(cam_sensor, 1);
        #endif
        #if defined(CAMERA_MODEL_ESP32S3_EYE)
        cam_sensor->set_vflip(cam_sensor, 1);
        #endif
        // Setup LED FLash if LED pin is defined in camera_pins.h
        #if defined(LED_GPIO_NUM)
        setupLedFlash(LED_GPIO_NUM);
        #endif

        return ESP_OK;
}

/*******************************************************************************
******** Setup helper function *************************************************
********************************************************************************/

static void setup_external_sensors()
{
        //configure temoerature/humidity temperature
        temperature_sensor_setup();
        //configure sound sensor
        sound_detector_enable();
}

static void temperature_sensor_setup()
{
        /* start temp-humd sensor */
        dht.begin();
        dht.readTemperature();
        dht.readHumidity();

         //timer interrupt to read temperature every 60s
        g_temp_hum_read_timer = timerBegin(0, 80, true);
        timerAttachInterrupt(g_temp_hum_read_timer, &read_temp_hum_isr, true);
        timerAlarmWrite(g_temp_hum_read_timer, TEMP_READ_PERIOD_S * 1000000, true);
        timerAlarmEnable(g_temp_hum_read_timer);
}

static void sound_detector_enable()
{
        pinMode(SOUND_MONITOR_PIN, INPUT);
        attachInterrupt(SOUND_MONITOR_PIN, sound_monitor_isr, FALLING);
}

static void sound_detector_disable()
{
        detachInterrupt(SOUND_MONITOR_PIN);
}

static String check_room_conditions(void)
{
        int temp;
        int humd;
        String ret = "room conditions ok";

        temp = dht.readTemperature();
        humd = dht.readHumidity();
        if (temp > TEMP_UPPER_LIMIT) {
                ret = "Status: Temperature(";
                ret += String(temp);
                ret +="C): Room is too hot \n";
        } else if (temp < TEMP_LOWER_LIMIT) {
                ret = "Status: Temperature(";
                ret += String(temp);
                ret +="C): Room is too cold \n";
        }

        if (humd > HUMD_UPPER_LIMIT) {
                ret = "Status: Humidity(";
                ret += String(humd);
                ret += "%): Level is too high \n";
        } else if (humd < HUMD_LOWER_LIMIT) {
                ret = "Status: Humidity(";
                ret += String(humd);
                ret += "%): Level is too low \n";
        }
        return ret;
}

static esp_err_t alert_open_na()
{
    return ESP_OK;
}

static esp_err_t send_alert_notification(String msg)
{
    send_ws_message("alert"); //web socket to reflect baby alert-state in baby button in browser
    return alert->send("Alert from BABY: \n" + msg + "Stream: 'http://" + WiFi.localIP().toString() + "'"); 
}

static esp_err_t print_alert_notification(String msg)
{
    Serial.println(msg);
    return ESP_OK;
}

static esp_err_t send_alert_via_whatsapp(String msg)
{
      String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + "&apikey=" + apiKey + "&text=" + urlEncode(msg);    
      HTTPClient http;
      http.begin(url);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      
      // Send HTTP POST request
      int httpResponseCode = http.POST(url);
      if (httpResponseCode == 200) {
              // Free resources
              http.end();
              return ESP_OK;
      } else {
              Serial.println("Error sending the message");
              Serial.print("HTTP response code: ");
              Serial.println(httpResponseCode);
              http.end();
              return ESP_FAIL;
      }
}

static esp_err_t send_alert_via_email(String msg)
{
        SMTPSession smtp;
        Session_Config config;
        SMTP_Message message;

        config.server.host_name = smtp_server;
        config.server.port = smtp_port;
        config.login.email = email_sender_account;
        config.login.password = email_sender_password;
        config.login.user_domain = "";

        // Set the sender email and name
        message.sender.name = "Smart Baby Monitor";
        message.sender.email = email_sender_account;

        // Set the recipient email
        message.addRecipient("User", email_recipient);

        // Set the subject and message
        message.subject = "Baby Monitor Alert";
        message.text.content = msg.c_str();

        //enable debugging
        smtp.debug(1);

        // Connect to the server
        smtp.connect(&config);

        // Send the email
        if (!MailClient.sendMail(&smtp, &message)) {
                Serial.println("Error sending Email, " + smtp.errorReason());
                return ESP_FAIL;
        }

        return ESP_OK;
}

/*******************************************************************************
******** GPIO ISRs *************************************************************
********************************************************************************/

void IRAM_ATTR read_temp_hum_isr() 
{
        BaseType_t res;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        /* set event group bit to notify monitor task */
        res = xEventGroupSetBitsFromISR(g_event_group, got_temp, &xHigherPriorityTaskWoken );

        /* Was the message posted successfully? */
        if( res != pdFAIL )
                /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
                switch should be requested. The macro used is port specific and will
                be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
                the documentation page for the port being used. */
                portYIELD_FROM_ISR( xHigherPriorityTaskWoken );      
};

void IRAM_ATTR sound_monitor_isr() {
        BaseType_t res;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        /* set event group bit to notify monitor loop */
        sound_detector_disable();
        res = xEventGroupSetBitsFromISR(g_event_group, got_sound, &xHigherPriorityTaskWoken );

        /* Was the message posted successfully? */
        if( res != pdFAIL )
                /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
                switch should be requested. The macro used is port specific and will
                be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
                the documentation page for the port being used. */
                portYIELD_FROM_ISR( xHigherPriorityTaskWoken );        
        
};
