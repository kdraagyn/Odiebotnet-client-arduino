#include "OdieBotnetDevice.h"
#include "OdieBotnetEnvironment.h"

#define DEBUGGING true

OdieBotnetClient odieBotnet( WIFI_SSID, WIFI_PASSWORD );
WebSocketsClient socket;

void eventHandler(WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSc] Disconnected!\n");
            break;
        case WStype_CONNECTED:
            {
                Serial.print("[WSc] Connected to url: ");
                Serial.println((char *)payload);

                // send message to server when Connected
		char* message = new char[500];
		sprintf(message, "{\"message\":\"Connected to server through web socket\", \"id\":%d}", odieBotnet.getId());
                socket.sendTXT(message);
            }
            break;
        case WStype_TEXT:
            Serial.print("[WSc] get text: ");
            Serial.println((char *)payload);
            // send message to server
            break;
        case WStype_BIN:
            Serial.print("[WSc] get binary length: ");
            Serial.println(length);
            break;
    }
}

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

  socket = odieBotnet.getSocket();
  socket.onEvent( eventHandler );
}

void loop() {
  socket.loop();
}
