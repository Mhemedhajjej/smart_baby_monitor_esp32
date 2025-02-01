const int got_temp = BIT0;
const int got_sound = BIT1;
const int got_baby_happy = BIT2;

#if defined(CAMERA_MODEL_WROVER_KIT)
#define TEMP_UPPER_LIMIT          22
#define TEMP_LOWER_LIMIT          16
#define HUMD_UPPER_LIMIT          50
#define HUMD_LOWER_LIMIT          30
#define TEMP_READ_PERIOD_S        1 * 60
#define DHTTYPE                   DHT11   // DHT 11
#define DHTPIN                    13     // Pin where the DHT11 is connected

#define SOUND_MONITOR_PIN         32
#endif