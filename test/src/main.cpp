#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#define FORMAT_SPIFFS_IF_FAILED true

const char *ssid = "ORBI";
const char *password = "ingeid/3510/1976";


const int relay = 15;

AsyncWebServer server(80);

void setup()
{
  //----------------------------------------------------Serial
  Serial.begin(115200);
  Serial.println("\n");

  //----------------------------------------------------GPIO
  
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);
  

  //----------------------------------------------------SPIFFS
  SPIFFS.begin(true);
  if(!SPIFFS.begin())
  {
    Serial.println("Erreur SPIFFS...");
    return;
  }

  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while(file)
  {
    Serial.print("File: ");
    Serial.println(file.name());
    file.close();
    file = root.openNextFile();
  }

  //----------------------------------------------------WIFI
  WiFi.begin(ssid, password);
	Serial.print("Tentative de connexion...");
	
	while(WiFi.status() != WL_CONNECTED)
	{
		Serial.print(".");
		delay(100);
	}
	
	Serial.println("\n");
	Serial.println("Connexion etablie!");
	Serial.print("Adresse IP: ");
	Serial.println(WiFi.localIP());

  //----------------------------------------------------SERVER
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/w3.css", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(SPIFFS, "/w3.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  server.on("/open", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(relay, HIGH);
    delay(5000);
    digitalWrite(relay, LOW);
    request->send(200);
  });


  server.begin();
  Serial.println("Serveur actif!");
}

void loop()
{

}