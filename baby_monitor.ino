#include "esp_camera.h"
#include <WiFi.h>

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15 
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well

// ===================
// Select camera model
// ===================
#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM
//#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
// ** Espressif Internal Boards **
//#define CAMERA_MODEL_ESP32_CAM_BOARD
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD
//#define CAMERA_MODEL_ESP32S3_CAM_LCD
//#define CAMERA_MODEL_DFRobot_FireBeetle2_ESP32S3 // Has PSRAM
//#define CAMERA_MODEL_DFRobot_Romeo_ESP32S3 // Has PSRAM
#include "camera_pins.h"

/******************************************************************* 
******* SYSTEM CONFIGURATIONS ************************************** 
*******************************************************************/

// Enter your WiFi credentials
const char* ssid = "SET_YOUR_WIFI_SSID_HERE";
const char* password = "SET_YOUR_WIFI_PASSWORD_HERE";

/******************************************************************* 
******* GLOBALS **************************************************** 
*******************************************************************/


/******************************************************************* 
******* STATIC FUNCTIONS ******************************************* 
*******************************************************************/

static void startCameraServer();
static void setupLedFlash(int pin);
static void esp32_cam_get_config(camera_config_t *config);
static void esp32_cam_get_config(camera_config_t *config);
static esp_err_t setup_camera();
static esp_err_t setup_camera_sensor();

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

        if (err == ESP_OK) {
                startCameraServer();
                Serial.print("Camera Server Ready! Use 'http://");
                Serial.print(WiFi.localIP());
                Serial.println("' to connect");
        }
}

void loop() 
{

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