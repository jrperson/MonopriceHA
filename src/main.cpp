#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include <SD.h>
#include <SPIFFS.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

AudioGeneratorMP3 *mp3 = NULL;
AudioOutputI2S *out = NULL;
AudioFileSourceHTTPStream *file_http = NULL;
AudioFileSourceBuffer *buff = NULL;

const char *ssid = "SSID";
const char *password = "PASSWORD";

#define RELAY_PIN 32
#define MAX_SRV_CLIENTS 1

AsyncWebServer server(80);
// WiFiServer serverTelnet(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

void stopPlaying() {
  digitalWrite(RELAY_PIN, LOW);

  if (mp3) {
    mp3->stop();
    delete mp3;
    mp3 = NULL;
  }
  if (buff) {
    buff->close();
    delete buff;
    buff = NULL;
  }
  if (file_http) {
    file_http->close();
    delete file_http;
    file_http = NULL;
  }
}

void setup(void) {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  Serial.begin(115200);
	// Serial1.begin(9600);

  delay(1000);

  WiFi.setHostname("MonopriceHA");
	WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		if(request->hasParam("url")) {
			String url = request->getParam("url")->value();
			url.replace(" ", "%20");
		  Serial.println(url);
		  int url_len = url.length() + 1;
		  char url_array[url_len];
		  url.toCharArray(url_array, url_len);

		  stopPlaying();

		  file_http = new AudioFileSourceHTTPStream(url_array);
		  buff = new AudioFileSourceBuffer(file_http, 16*1024);
		  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
		  mp3 = new AudioGeneratorMP3();
		  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
		  mp3->begin(buff, out);
      digitalWrite(RELAY_PIN, HIGH);
		}

    request->send(200, "text/plain", "hello from esp32!");
  });

	// serverTelnet.begin();
  server.begin();
  Serial.println("HTTP server started");

  out = new AudioOutputI2S();
  out->SetPinout(26,25,32);
  out->SetGain(0.5);
}

void loop(void) {
  if(mp3 && mp3->isRunning()) {
    Serial.println("playing...");
    if (!mp3->loop()) {
      Serial.println("ended");
      stopPlaying();
    }
  } //else {
	// 	uint8_t i;
  //   if (serverTelnet.hasClient()){
  //     for(i = 0; i < MAX_SRV_CLIENTS; i++){
  //       //find free/disconnected spot
  //       if (!serverClients[i] || !serverClients[i].connected()){
  //         if(serverClients[i]) serverClients[i].stop();
  //         serverClients[i] = serverTelnet.available();
  //         Serial.print("New client: ");
  //         Serial.print(i); Serial.print(' ');
  //         Serial.println(serverClients[i].remoteIP());
  //         break;
  //       }
  //     }
  //     if (i >= MAX_SRV_CLIENTS) {
  //       //no free/disconnected spot so reject
  //       serverTelnet.available().stop();
  //     }
  //   }
  //   //check clients for data
  //   for(i = 0; i < MAX_SRV_CLIENTS; i++){
  //     if (serverClients[i] && serverClients[i].connected()){
  //       if(serverClients[i].available()){
  //         //get data from the telnet client and push it to the UART
  //         while(serverClients[i].available()) Serial1.write(serverClients[i].read());
  //       }
  //     }
  //     else {
  //       if (serverClients[i]) {
  //         serverClients[i].stop();
  //       }
  //     }
  //   }
  //   //check UART for data
  //   if(Serial1.available()){
  //     size_t len = Serial1.available();
  //     uint8_t sbuf[len];
  //     Serial1.readBytes(sbuf, len);
  //     //push UART data to all connected telnet clients
  //     for(i = 0; i < MAX_SRV_CLIENTS; i++){
  //       if (serverClients[i] && serverClients[i].connected()){
  //         serverClients[i].write(sbuf, len);
  //         delay(1);
  //       }
  //     }
  //   }
	// 	delay(1000);
  // }
}
