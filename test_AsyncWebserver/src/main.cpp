#include <Arduino.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "SPIFFS.h"

#define CAMERA_MODEL_AI_THINKER
#include "cameraAPI.h"

#include "home_wifi_multi.h"


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//boolean takeNewPhoto = false;
const int led = 4;
const int relay = 15;

// // Photo File Name to save in SPIFFS
// #define FILE_PHOTO "/photo.jpg"


void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  #ifdef CAMERA
    cameraSetup();
  #endif  

  // Connect to Wi-Fi
  WiFi.begin(SSID1, PWD1);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  

  // Print ESP32 Local IP Address
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());

  //GPIO
  pinMode(led, OUTPUT);
  pinMode (relay, OUTPUT);
  digitalWrite(led, LOW);
  digitalWrite(relay, LOW);

  //SPIFFS
  if(!SPIFFS.begin())
  {
    Serial.println("Erreur SPIFFS...");
    return;
  }

//Permet de lire tous les fichiers contenu dans le spiffs et affiche le nom du fichier dans la console, pour voir s'il y a tout ce qu'il faut
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while(file)
  {
    Serial.print("File: ");
    Serial.println(file.name());
    file.close();
    file = root.openNextFile();
  }


//Configuration du serveur Web.
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(relay, HIGH);
    request->send(200);
  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(relay, LOW);
    request->send(200);
  });

  // server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
  //   takeNewPhoto = true;
  //   request->send_P(200, "text/plain", "Taking Photo");
  // });

  #ifdef CAMERA
    server.on("/stream", HTTP_GET, handleStream);
  #endif

  // server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest * request) {
  //   request->send(SPIFFS, FILE_PHOTO, "image/jpg", false);
  // });

  // Start server
  server.begin();
  Serial.println("Serveur actif !");

}

void loop() {
  // if (takeNewPhoto) {
  //   //capturePhotoSaveSpiffs();
  //   takeNewPhoto = false;
  // }
  delay(1);
}

// Check if photo capture was successful
// bool checkPhoto( fs::FS &fs ) {
//  // File f_pic = fs.open( FILE_PHOTO );
//   unsigned int pic_sz = f_pic.size();
//   return ( pic_sz > 100 );
// }

// Capture Photo and Save it to SPIFFS
//void capturePhotoSaveSpiffs( void ) {
//  camera_fb_t * fb = NULL; // pointer
//  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  // do {
  //   // Take a photo with the camera
  //   Serial.println("Taking a photo...");

 //    fb = esp_camera_fb_get();
  //   if (!fb) {
  //     Serial.println("Camera capture failed");
  //     return;
  //   }

  //   // Photo file name
  //   // Serial.printf("Picture file name: %s\n", FILE_PHOTO);
  //   // File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

  //   // Insert the data in the photo file
  //   // if (!file) {
  //   //   Serial.println("Failed to open file in writing mode");
  //   // }
  //   // else {
  //   //   file.write(fb->buf, fb->len); // payload (image), payload length
  //   //   Serial.print("The picture has been saved in ");
  //   //   Serial.print(FILE_PHOTO);
  //   //   Serial.print(" - Size: ");
  //   //   Serial.print(file.size());
  //   //   Serial.println(" bytes");
  //   // }
  //   // Close the file
  //   file.close();
  //   esp_camera_fb_return(fb);

  //   // check if file has been correctly saved in SPIFFS
  //   ok = checkPhoto(SPIFFS);
  // } while ( !ok );
//}