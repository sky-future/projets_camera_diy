#ifndef H_AD_CAM_PROJECT
#define H_AD_CAM_PROJECT

#include "Arduino.h"
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include "esp_http_server.h"
#include <SPIFFS.h>

#include "MotionDetection.h"

/* macros ============================================================== */
/* constants =========================================================== */

#define CAMERA_MODEL_AI_THINKER
  #define PWDN_GPIO_NUM     32 //Nous definissons les pins de la carte.
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22

  #define Flashlight         4

#define PART_BOUNDARY "123456789000000000000987654321"

// define the source format and the downscaled dimensions
#define FRAME_SIZE FRAMESIZE_SVGA //1600x1200:il lag (2-3 sec de latence) + 320x240: aucune image envoyee par boucle stream (???)
#define PIXFORMAT PIXFORMAT_JPEG //il reboot avec le format PIXFORMAT_GRAYSCALE
#define W 800
#define H 600
#define CHANNELS 1
#define w 40
#define h 30
// tweak these values as per your need
#define BLOCK_VARIATION_THRESHOLD 0.3 // Set how much a pixel value should differ to be considered as a change. Value is in range [0-255] or [0-1].
#define MOTION_THRESHOLD 0.07 // Set how many pixels (in percent) should change to be considered as motion
//#define MOTION_TIME 300 // Temps[ms] minimum entre chaque image prise --> Obsolete

extern const char* ssid;
extern const char* password;

// the motion detection algorithm
extern Eloquent::Vision::MotionDetection<w, h> motion;

/* types =============================================================== */
/* structures ========================================================== */
/* internal public functions =========================================== */

void camera_streamConfiguration();
void setup_wifi();
void init_stream_handler();
void startCameraServer();
void motionDetection();

/* entry points ======================================================== */
/* public variables ==================================================== */
/* inline public functions  ============================================ */

#endif