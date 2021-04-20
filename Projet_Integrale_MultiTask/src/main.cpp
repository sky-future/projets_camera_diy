#include "CAM_Project.h"

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
 
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  SPIFFS.begin(true);
  
  camera_streamConfiguration();

  motion.setBlockVariationThreshold(BLOCK_VARIATION_THRESHOLD);

  // Wi-Fi connection
  setup_wifi();
  
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.print(WiFi.localIP());

  init_stream_handler();
  // Start streaming web server
  startCameraServer();
}

void loop() {
  //MotionDetection Boucle
  motionDetection();
  

}