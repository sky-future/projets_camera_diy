#ifndef H_AD_CAMERA_FUNCTION
#define H_AD_CAMERA_FUNCTION

#include "CAM_Project.h"

//Replace with your network credentials
const char* ssid = "ORBI";
const char* password = "ingeid/3510/1976";
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
camera_fb_t *fb = NULL; //Structure des donnees du tampon d image de la camera.

QueueHandle_t queue_to_motionDetection, queue_for_fb;
//MotionDetection takes ~650 ms, stream_handler takes ~75 ms,
//so stream_handler has the time to send 10 messages
int queue_size = 15;

// the downscaler algorithm
// for more details see https://eloquentarduino.github.io/2020/05/easier-faster-pure-video-esp32-cam-motion-detection
Eloquent::Vision::ImageProcessing::DownscaleStrategies::Center<W, H, w, h> centerStrategy;
// the downscaler container
Eloquent::Vision::ImageProcessing::Downscale::Downscaler<W, H, CHANNELS, w, h> downscaler(&centerStrategy);
// the motion detection algorithm
Eloquent::Vision::MotionDetection<w, h> motion;

uint8_t *downscaled = NULL; // the buffer to store the downscaled version of the image
uint8_t *_rgb888_buf = NULL;

//Initialise la camera avec la configuration requise par le flux stream
void camera_streamConfiguration(){

  camera_config_t config; //Structure de configuration pour l initialisation de la camera
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT;

    if(psramFound()){
    config.frame_size = FRAME_SIZE;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAME_SIZE;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

// Wi-Fi connection
void setup_wifi(){

  	Serial.printf("Connecting to %s ", ssid);
  	WiFi.begin(ssid, password);
  	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
  	Serial.println("WiFi connected");
}

//Display the used HEAP and PSRAM
void logMemory(String message = "") {
  Serial.println(message);

  Serial.print("Used HEAP: ");
  Serial.println(ESP.getHeapSize() - ESP.getFreeHeap());

  Serial.print("Used PSRAM: ");
  Serial.println(ESP.getPsramSize() - ESP.getFreePsram());
}

/*
Function which takes a measure of the time which passed between 2 appels of itself
*/
uint32_t timeMesure(uint32_t &tmp, String comment = "")
{
  uint32_t temps = tmp;

  if(tmp == 0){
    tmp = millis();
  }
  else{
    Serial.print(comment);
    Serial.println("");
    Serial.print("Time measured = ");
    Serial.print(millis() - tmp);
    Serial.println(" [ms]");
    tmp = 0;
  }
  return (uint32_t) (millis() - temps);
}

void copy_buf(uint8_t * scr_buf, size_t scr_buf_len, uint8_t * &new_buf)
{

  for(int i=0 ; i < scr_buf_len ; i++)
  {
    new_buf[i] = scr_buf[i];
  }
}

//Function which converts a pixel from RGB888 to GRAYSCALE
uint8_t rgb888_to_gray(uint8_t *pixel)
{
  uint8_t blue = *(pixel);
  uint8_t green = *(pixel + 1);
  uint8_t red = *(pixel + 2);

  return (uint8_t) ((blue + green + red )/3);
}

/*
Function which takes an image
*/
camera_fb_t* capture_camera()
{
  camera_fb_t *fb = NULL;
  
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed xx1");
  }

  return fb;
}

void debug_affichage(String comment)
{
  //Serial.println(comment);
}

/*
Return true if stream_handler runs, false otherwise.
*/
bool stream_handler_state()
{
  int8_t message;
  bool state = false; //if stream_handler doesn't send message, we will return false -> doesn't run

  while(uxQueueMessagesWaiting(queue_to_motionDetection) != 0){
    
    xQueueReceive(queue_to_motionDetection, &message, portMAX_DELAY);

    state = true;
    //message = 1: means stream_handler begins
    //message = 2: means stream_handler always runs
    //=> receive a message means stream_handler is active.
  }

  return state;
}

void motionDetection()
{
  uint8_t * _jpg_buf = NULL;
  size_t _jpg_buf_len = 0;
  pixformat_t buf_pixformat;
  int8_t message, nbr;
  uint32_t start = millis();
  

  if(fb) {esp_camera_fb_return(fb); fb = NULL;} //stream_handler could be stop before its esp_camera_return() line
  debug_affichage("chemin 2");

  while (!stream_handler_state()) // On entre si stream_handler ne tourne pas
  {
    //if(millis() - start >= MOTION_TIME){ //On prend une image pour la detection de mouvement toutes les MOTION_TIME
    //=> commenté car le delai est inutile vu que la detection prend elle-meme un certain temps

      fb = esp_camera_fb_get(); //50 ms
      if (!fb) {
        Serial.println("Camera capture failed");
      } else {
        start = millis();

        if(fmt2rgb888(fb->buf, fb->len,fb->format, _rgb888_buf)){ //615 ms

          esp_camera_fb_return(fb);
          fb = NULL;

          // scale image from size H * W to size h * w
          downscaler.downscale(_rgb888_buf, downscaled); //50 ms
          motion.update(downscaled); //1 ms
          // detect motion on the downscaled image
          motion.detect(); //0-1 ms

          Serial.print("Ratio = ");
          Serial.println( motion.ratio());

          if (motion.ratio() > MOTION_THRESHOLD) {
            Serial.println("Motion detected");
    
            // here we can take a picture of the motion
          }
        }
      }
    //}
  }
  debug_affichage("chemin 3");
  
  do //On entre puisque stream_handler tourne (on est sorti de la boucle precedante)
  { debug_affichage("chemin 4");
    //if(millis() - start >= MOTION_TIME){ //On prend une image pour la detection de mouvement toutes les MOTION_TIME
      
      message = 1; //Say to stream_handler that motionDetection needs frame buf
      xQueueSendToFront(queue_for_fb, &message, portMAX_DELAY);
      start = millis();
      nbr = uxQueueMessagesWaiting(queue_for_fb);
      while(uxQueueMessagesWaiting(queue_for_fb) == nbr){ //waiting until received a message back
        if(millis()-start > 300) {debug_affichage("chemin 5"); return;} //After 300 ms, we suppose stream_handler doesn't run anymore (It needs 80 ms to run one times)
       
      }
      debug_affichage("chemin 6");

      _jpg_buf = (uint8_t *) ps_malloc(fb->len);
      copy_buf(fb->buf, fb->len, _jpg_buf); //7-10 ms
      _jpg_buf_len = fb->len;
      buf_pixformat = fb->format;
      if(_jpg_buf == NULL)Serial.println("_jpg_buf est nul !!");
      start = millis();

      if(fmt2rgb888(_jpg_buf, _jpg_buf_len, buf_pixformat, _rgb888_buf)){ //615 ms
        debug_affichage("chemin 6'");
        // scale image from size H * W to size h * w
        downscaler.downscale(_rgb888_buf, downscaled); //50 ms
        motion.update(downscaled); //1 ms
        // detect motion on the downscaled image
        motion.detect(); //0-1 ms

        Serial.print("Ratio = ");
        Serial.println( motion.ratio());

        if (motion.ratio() > MOTION_THRESHOLD) {
          Serial.println("Motion detected");
    
          // here we can take a picture of the motion
        }
      }
      debug_affichage("chemin 7");
      free(_jpg_buf);
      _jpg_buf = NULL;
      _jpg_buf_len = 0;
    //}
  } while (stream_handler_state());
}

/*
   This method only stream one JPEG image - Cette méthode ne publie qu'une seule image JPEG
   Compatible with/avec Jeedom / NextDom / Domoticz
*/

static esp_err_t capture_handler(httpd_req_t *req) {
  esp_err_t res = ESP_OK;
  size_t fb_len = 0;
  int64_t fr_start = esp_timer_get_time();

  res = httpd_resp_set_type(req, "image/jpeg");
  if (res == ESP_OK)
  {
    res = httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=image.jpg");  //capture
  }
  if (res == ESP_OK) {
    ESP_LOGI(TAG, "Take a picture");
    //while(1){
    fr_start = esp_timer_get_time();
    fb = esp_camera_fb_get();
    if (!fb)
    {
      ESP_LOGE(TAG, "Camera capture failed");
      httpd_resp_send_500(req);
      return ESP_FAIL;
    } else {
      fb_len = fb->len;
      res = httpd_resp_send(req, (const char *)fb->buf, fb->len);

      esp_camera_fb_return(fb);
      // Uncomment if you want to know the bit rate - décommentez pour connaître le débit
      int64_t fr_end = esp_timer_get_time();
      Serial.printf("JPG: %uKB %ums", (uint32_t)(fb_len / 1024), (uint32_t)((fr_end - fr_start) / 1000));
      return res;
    }
    //}
  }
}

/*
Function which initializes the stream_handler function.
This method to allocate the memory is used to avoid a stack overflow in stream_handler.
It avoids the error "Stack canary watchpoint triggered".
*/
void init_stream_handler()
{
  downscaled = (uint8_t *) ps_malloc(w*h); // the buffer to store the downscaled version of the image
  _rgb888_buf = (uint8_t *) ps_malloc(3*W*H);

  queue_to_motionDetection = xQueueCreate( queue_size, sizeof( int8_t ) ); //Way to communicate between tasks
  if(queue_to_motionDetection == NULL) Serial.println("Error creating the queue");

  queue_for_fb = xQueueCreate( queue_size, sizeof( int8_t ) ); //Way to communicate between tasks
  if(queue_for_fb == NULL) Serial.println("Error creating the queue");
}

/*
Fonction qui publie une suite d image JPEG + indique lorsqu un mouvement est detecte
*/
static esp_err_t stream_handler(httpd_req_t *req){ //Structure des donnees de requête HTTP.

  if(fb){esp_camera_fb_return(fb); fb = NULL;} //stream_handler could be stop before the esp_camera_return() line and begins again

  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  debug_affichage("chemin 8");
  int8_t message = 1;
  xQueueSendToFront(queue_to_motionDetection, &message, portMAX_DELAY); //Say to MotionDetection that stream_handler begins
  delay(700);//Give time to MotionDetection to realize stream_handler began
  debug_affichage("chemin 9");

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }
  logMemory();

  while(true){ //Renvoye des images JPEG jusqu a ce qu un envoi echoue.
    debug_affichage("chemin 10");
    message = 2;
    if(!xQueueSendToFront(queue_to_motionDetection, &message, portMAX_DELAY))Serial.println("queue_to_motionDetection failed");; //Say to MotionDetection that stream_handler always runs
    debug_affichage("chemin 11");
    //logMemory();
    fb = esp_camera_fb_get(); //50 ms
    if (!fb) {
      Serial.println("Camera capture failed xx2");
      res = ESP_FAIL;
    } else {

      if(uxQueueMessagesWaiting(queue_for_fb) >= 1) //If a message is waiting, it means MotionDetection needs fb
      {debug_affichage("chemin 12");
        xQueueSendToFront(queue_for_fb, &message, portMAX_DELAY);//The value of the message does not matter
        delay(10); //let the time to MotionDetection to copy fb
        if(!xQueueReset(queue_for_fb)) { Serial.println("queue_for_fb reset failed"); //clear the queue
          while(uxQueueMessagesWaiting(queue_for_fb) != 0) xQueueReceive(queue_for_fb, &message, portMAX_DELAY); //If clear the queue failed, we clear the queue manually
        }
      }
      debug_affichage("chemin 13");

      if(fb->width > 400){ //Si la largeur du tampon en pixels est >400, on compresse en format JPEG.
        if(fb->format != PIXFORMAT_JPEG){ //Si le format n est pas en JPEG, on le met en JPEG
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else { //Si le format est deja en JPEG, on ne reformate pas
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
    debug_affichage("chemin 14");
  }
  debug_affichage("chemin 15");
  return res;
}

void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.task_priority = 1;
  config.stack_size = 10000;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  
  //Serial.printf("Starting web server on port: '%d'\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
  }
}

#endif