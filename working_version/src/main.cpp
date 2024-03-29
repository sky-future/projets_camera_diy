/*********
  
  IMPORTANT BEFORE TO DOWNLOAD SKETCH !!!
   - Install ESP32 libraries
   - Select Board "ESP32 Wrover Module"
   - Select the Partion Scheme "Huge APP (3MB No OTA)"
   - GPIO 0 must be connected to GND to upload a sketch
   - After connecting GPIO 0 to GND, press the ESP32-CAM on-board RESET button to put your board in flashing mode
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include <esp_event_loop.h>
#include <esp_log.h>
#include "esp_timer.h"
#include "esp_camera.h"

#include <WiFi.h>
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems
//#include "dl_lib.h"
#include "esp_http_server.h" // API https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/protocols/esp_http_server.html

#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

//Replace with your network credentials - Remplacez par vos identificants de connexion WiFi
const char *ssid = "ORBI";
const char *password = "ingeid/3510/1976";

//Declare la pin du relay dans une variable
const int relay = 15;

//variable pour mesurer le temps entre plusierus actions
int eventTime;
int eventTime2;
int interval;

#define SERIAL_DEBUG true             // Enable / Disable log - activer / désactiver le journal
#define ESP_LOG_LEVEL ESP_LOG_VERBOSE // ESP_LOG_NONE, ESP_LOG_VERBOSE, ESP_LOG_DEBUG, ESP_LOG_ERROR, ESP_LOG_WARM, ESP_LOG_INFO

// Web server port - port du serveur web
#define WEB_SERVER_PORT 80
#define URI_STATIC_JPEG "/jpg/image.jpg"
#define URI_STREAM "/stream"

// Basic image Settings (compression, flip vertical orientation) - Réglages basiques de l'image (compression, inverse l'orientation verticale)
#define FLIP_V true          // Vertical flip - inverse l'image verticalement
#define MIRROR_H true        // Horizontal mirror - miroir horizontal
#define IMAGE_COMPRESSION 10 //0-63 lower number means higher quality - Plus de chiffre est petit, meilleure est la qualité de l'image, plus gros est le fichier

static const char *TAG = "esp32-cam";

/*
   Handler for video streaming - Entête pour le flux vidéo
*/
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// Uncomment your dev board model - Décommentez votre carte de développement
// This project was only tested with the AI Thinker Model - le croquis a été testé uniquement avec le modèle AI Thinker
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_WROVER_KIT)
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 21
#define SIOD_GPIO_NUM 26 //Flash LED - Flash Light is On if SD card is present
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 19
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 5
#define Y2_GPIO_NUM 4
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM 27
#define SIOD_GPIO_NUM 25
#define SIOC_GPIO_NUM 23
#define Y9_GPIO_NUM 19
#define Y8_GPIO_NUM 36
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 39
#define Y5_GPIO_NUM 5
#define Y4_GPIO_NUM 34
#define Y3_GPIO_NUM 35
#define Y2_GPIO_NUM 32
#define VSYNC_GPIO_NUM 22
#define HREF_GPIO_NUM 26
#define PCLK_GPIO_NUM 21

#elif defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26 //Flash LED - Flash Light is On if SD card is present
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#else
#error "Camera model not selected"
#endif
#define Flashlight 4

httpd_handle_t stream_httpd = NULL;

boolean flag_b = false;

/*
   This method only stream one JPEG image - Cette méthode ne publie qu'une seule image JPEG
   Compatible with/avec Jeedom / NextDom / Domoticz
*/
static esp_err_t capture_handler(httpd_req_t *req)
{
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t fb_len = 0;
  int64_t fr_start = esp_timer_get_time();

  res = httpd_resp_set_type(req, "image/jpeg");
  if (res == ESP_OK)
  {
    res = httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=image.jpg"); //capture
  }
  if (res == ESP_OK)
  {
    ESP_LOGI(TAG, "Take a picture");
    digitalWrite(Flashlight, HIGH);
    //while(1){
    fr_start = esp_timer_get_time();
    fb = esp_camera_fb_get();
    if (!fb)
    {
      ESP_LOGE(TAG, "Camera capture failed");
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    else
    {
      fb_len = fb->len;
      res = httpd_resp_send(req, (const char *)fb->buf, fb->len);

      esp_camera_fb_return(fb);
      // Uncomment if you want to know the bit rate - décommentez pour connaître le débit
      //int64_t fr_end = esp_timer_get_time();
      //ESP_LOGD(TAG, "JPG: %uKB %ums", (uint32_t)(fb_len / 1024), (uint32_t)((fr_end - fr_start) / 1000));
      digitalWrite(Flashlight, LOW);
      return res;
    }
    //}
  }
}

/*
   This method stream continuously a video
   Compatible with/avec Home Assistant, HASS.IO
*/
esp_err_t stream_handler(httpd_req_t *req)
{
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len;
  uint8_t *_jpg_buf;
  char *part_buf[64];
  static int64_t last_frame = 0;
  if (!last_frame)
  {
    last_frame = esp_timer_get_time();
  }

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK)
  {
    return res;
  }
  ESP_LOGI(TAG, "Start video streaming");
  //digitalWrite(Flashlight, HIGH);

  while (true)
  {
    fb = esp_camera_fb_get();
    if (!fb)
    {
      ESP_LOGE(TAG, "Camera capture failed");
      res = ESP_FAIL;
    }
    else
    {
      if (fb->format != PIXFORMAT_JPEG)
      {
        bool jpeg_converted = frame2jpg(fb, 64, &_jpg_buf, &_jpg_buf_len);
        if (!jpeg_converted)
        {
          ESP_LOGE(TAG, "JPEG compression failed");
          esp_camera_fb_return(fb);
          res = ESP_FAIL;
        }
      }
      else
      {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
    }
    if (res == ESP_OK)
    {
      eventTime = millis(); //met le temps T dans une variable
      size_t hlen = snprintf((char *)part_buf, 80, _STREAM_PART, _jpg_buf_len); //Utilise un emplacement memoire pour le buf

      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK)
    {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len); //
    }
    if (res == ESP_OK)
    {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY)); //renvoi le jpg buf vers la page web
      //digitalWrite(Flashlight, HIGH);
      eventTime2 = millis(); //met le temps T dans une seconde variable
      interval = eventTime - eventTime2;
      Serial.println(interval);

    }
    if (res == ESP_OK && !flag_b)
    {
      //digitalWrite(Flashlight, HIGH);
      flag_b = true;
    }
    if (fb->format != PIXFORMAT_JPEG)
    {
      free(_jpg_buf);
    }
    esp_camera_fb_return(fb);
    if (res != ESP_OK)
    {
      digitalWrite(Flashlight, LOW);
      flag_b = false;
      break;
    }

    //Uncomment if you want to know the bit rate - décommentez pour connaître le débit
    /*
      int64_t fr_end = esp_timer_get_time();
      int64_t frame_time = fr_end - last_frame;
      last_frame = fr_end;
      frame_time /= 1000;
      ESP_LOGD(TAG, "MJPG: %uKB %ums (%.1ffps)",
        (uint32_t)(_jpg_buf_len/1024),
        (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    */
  }

  last_frame = 0;
  return res;
}

esp_err_t command_handler(httpd_req_t *req)
{
  char *buf;
  size_t buf_len;
  char variable[32] = {
      0,
  };
  char value[32] = {
      0,
  };

  buf_len = httpd_req_get_url_query_len(req) + 1;

  if (buf_len > 1)
  {
    buf = (char *)malloc(buf_len);
    if (!buf)
    {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
    {
      Serial.println("Valeur de buf :");
      Serial.println(buf);
      if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
          httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK)
      {
      }
      else
      {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    }
    else
    {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  }
  else
  {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  int val = atoi(value);
  //sensor_t * s = esp_camera_sensor_get();
  int res = 0;

  //Control Relay
  if (!strcmp(variable, "relay"))
  {
    digitalWrite(relay, val);
  }
  else
  {
    res = -1;
  }

  if (res)
  {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!doctype html>
  <html>
  <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <meta http-equiv="Expires" Content="0">
      <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.0.13/css/all.css"
        integrity="sha384-DNOHZ68U8hZfKXOrtjWvjxusGo9WQnrNx2sqG0tfsghAvtVlRW3tvkXWZh58N9jp" crossorigin="anonymous" />
      <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.1.1/css/bootstrap.min.css"
        integrity="sha384-WskhaSGFgHYWDcbwN70/dfYBj47jz9qbsMId/iRN3ewGhXQFZCSftd1LZCfmhktB" crossorigin="anonymous" />
  </head>
  <body onload="document.getElementById('stream').src=location.origin+':81/stream';">
  <header id="main-header" class="py-2 bg-success text-white">
    <div class="container">
      <div class="row justify-content-md-center">
        <div class="col-md-6 text-center">
          <h1><i class="fas fa-cog"></i> ESP32Cam control ouverture de porte</h1>
        </div>
      </div>
    </div>
  </header>
  <section class="bg-white">
    <div class="container">
      <div class="row">
      <div class="col">
          <div class="card bg-light m-2">
            <div class="card-header">localhost/stream </div>
            <div class="card-body">
              <h5 class="card-title">Camera</h5>
              <img id="stream" style="margin-top: 50px; width:400px"></img><br>
            </div>
          </div>
        </div>
      
      <div class="col">
        <button type="button" class="btn btn-lg btn-success" onclick="fetch(location.origin+'/open?var=relay&val=1');">Relay On</button>
        <button type="button" class="btn btn-lg btn-danger" onclick="fetch(location.origin+'/open?var=relay&val=0');">Relay off</button>
      </div>
      </div>
      
    </div>
  </section>
  
  
  </body>
  </html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}

void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = WEB_SERVER_PORT;

  // endpoints
  static const httpd_uri_t static_image = {
      .uri = URI_STATIC_JPEG,
      .method = HTTP_GET,
      .handler = capture_handler,
      .user_ctx = NULL};

  static const httpd_uri_t stream_video = {
      .uri = URI_STREAM,
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL};

  //test relay
  static const httpd_uri_t uri_get = {
      .uri = "/open",
      .method = HTTP_GET,
      .handler = command_handler,
      .user_ctx = NULL};

  static const httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
      .user_ctx = NULL};

  ESP_LOGI(TAG, "Register URIs and start web server");
  if (httpd_start(&stream_httpd, &config) == ESP_OK)
  {
    if (httpd_register_uri_handler(stream_httpd, &static_image) != ESP_OK)
    {
      ESP_LOGE(TAG, "register uri failed for static_image");
      return;
    };
    if (httpd_register_uri_handler(stream_httpd, &stream_video) != ESP_OK)
    {
      ESP_LOGE(TAG, "register uri failed for stream_video");
      return;
    };
    if (httpd_register_uri_handler(stream_httpd, &uri_get) != ESP_OK)
    {
      ESP_LOGE(TAG, "register uri failed for uri_get");
      return;
    };
    if (httpd_register_uri_handler(stream_httpd, &index_uri) != ESP_OK)
    {
      ESP_LOGE(TAG, "register uri failed for index_uri");
      return;
    };
  }

  config.server_port += 1; //Stream Port
  config.ctrl_port += 1;   //UDP Port
  Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK)
  {
    httpd_register_uri_handler(stream_httpd, &stream_video);
  }
}

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  pinMode(Flashlight, OUTPUT);
  pinMode(relay, OUTPUT);
  Serial.setDebugOutput(SERIAL_DEBUG);
  esp_log_level_set("*", ESP_LOG_LEVEL);



  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;       //XCLK 20MHz or 10MHz
  config.pixel_format = PIXFORMAT_JPEG; //YUV422,GRAYSCALE,RGB565,JPEG
  config.frame_size = FRAMESIZE_SVGA;   //UXGA SVGA VGA QVGA Do not use sizes above QVGA when not JPEG
  config.jpeg_quality = 10;
  config.fb_count = 2; //if more than one, i2s runs in continuous mode. Use only with JPEG

  // Camera init - Initialise la caméra
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
    return;
  }
  else
  {
    ESP_LOGD(TAG, "Camera correctly initialized ");
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, FLIP_V);
    s->set_hmirror(s, MIRROR_H);
  }

  // Wi-Fi connection - Connecte le module au réseau Wi-Fi
  ESP_LOGD(TAG, "Start Wi-Fi connexion ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  ESP_LOGD(TAG, "Wi-Fi connected ");

  // Start streaming web server
  startCameraServer();

  ESP_LOGI(TAG, "Camera Stream Ready");
  Serial.println(WiFi.localIP());
}

void loop()
{
  delay(1);

}