#include "OdieBotnetDevice.h"

#define WIFI_SSID "OdisWiFi"
#define WIFI_PASSWORD "underwaterhorsesarecalledseahorses"

#define DEBUGGING true

OdieBotnetClient odieBotnet( WIFI_SSID, WIFI_PASSWORD );
void setup() {
  Serial.begin(115200);
  Serial.println("Starting connection");
  while(!odieBotnet.connect()) {
    Serial.println("ESP3266 didn't connect, trying again...");
    delay(200);
  }
  Serial.print("Connected to ");
  Serial.print( WIFI_SSID );
  Serial.println( "!" );
}

void loop() {
  // put your main code here, to run repeatedly:
  char* message;
  sprintf( message, "{\"message\":\"pew pew pew\", \"id\":%d}", odieBotnet.getId());
  odieBotnet.send(message);
  delay(1000);
}
