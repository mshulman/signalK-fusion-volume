/*
  mshulman@gmail.com
  25-Jul-2021

Subscribe to the SignalK server and get the volume control settings
  for all zones

  Eventually, display the volume on a local display, and allow changing
  the volume with a rotary encoder
*/
#include <ArduinoHttpClient.h>
#include <WiFi101.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"
#include <Regexp.h>

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
/////// WiFi Settings ///////
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)


char serverAddress[] = "192.168.1.250";  // server address
int port = 3000;

WiFiClient wifi;
WebSocketClient client = WebSocketClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;
int count = 0;

void setUpSubscription() {
  client.begin("/signalk/v1/stream?subscribe=none");

  while (!client.connected()) {
    ; // wait to connect
  }

  client.beginMessage(TYPE_TEXT);
  client.print("{\"context\":\"vessels.self\",\"subscribe\": [{\"path\": \"entertainment.device.fusion1.output.*\"}]}");
  client.endMessage();

}

void setup() {
  WiFi.setPins(8,7,4,2);
  Serial.begin(9600);
  Serial.println("Starting...");
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
  }

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.println("starting WebSocket client");
  
}

void printVolumeUpdates(String message) {
  StaticJsonDocument<512> doc;
  
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  
  const char* context = doc["context"];
  
  JsonObject updates_0 = doc["updates"][0];
  
  JsonObject updates_0_source = updates_0["source"];
  const char* updates_0_source_label = updates_0_source["label"]; // "fusion"
  const char* updates_0_source_type = updates_0_source["type"]; // "NMEA2000"
  long updates_0_source_pgn = updates_0_source["pgn"]; // 130820
  const char* updates_0_source_src = updates_0_source["src"]; // "10"
  
  const char* updates_0__source = updates_0["$source"]; // "fusion.10"
  const char* updates_0_timestamp = updates_0["timestamp"]; // "2017-04-15T18:24:26.782Z"
  
  const char* path = updates_0["values"][0]["path"];
  int volume = updates_0["values"][0]["value"]; // 10

  Serial.println();
  Serial.print("path: ");

  char* completePath = const_cast<char*>(path);
  
  MatchState ms;
  ms.Target(completePath);
  char result = ms.Match ("zone.");

  if (result > 0)  {
    String zone = String(completePath).substring(ms.MatchStart, ms.MatchStart + ms.MatchLength);
    Serial.print("Zone: ");
    Serial.println(zone);
  } else {
    Serial.println ("No match.");
  }
    
  Serial.print("volume: ");
  Serial.println(volume);
}

static void printDots() {
  if (count > 50) {
    Serial.println(".");
    count = 0;
  } else {
    Serial.print(".");
  }
}
  

void loop() {
  setUpSubscription();
  while (client.connected()) {
    // increment count for next message
    count++;
  
    // check if a message is available to be received
    int messageSize = client.parseMessage();
  
    if (messageSize > 0) {
      const String message = client.readString();
      // look for entertainment.device.fusion1.output.zone1.volume.master
      if (message.indexOf("volume") > -1) {
        printVolumeUpdates(message);
      } else {
        printDots();
      }
    }
    // wait 100 msec
    delay(100);
  }
  Serial.println("client disconnected");
}
