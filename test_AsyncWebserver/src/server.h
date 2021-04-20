//   #include "ESPAsyncWebServer.h"

//   // Create AsyncWebServer object on port 80
// AsyncWebServer server(80);
  
//     server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
//   {
//     digitalWrite(relay, HIGH);
//     digitalWrite(led, HIGH);
//     request->send(200);
//   });

//   server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
//   {
//     digitalWrite(relay, LOW);
//     digitalWrite(led, LOW);
//     request->send(200);
//   });