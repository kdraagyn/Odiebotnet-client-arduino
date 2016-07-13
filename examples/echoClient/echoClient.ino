#include "OdiebotnetClient.h"
#include "OdiebotnetConfig.h"

// Used to give meaningful serial output
//	Serial MUST be initialized before odieBotnet is connected or used
#define DEBUGGING true

OdiebotnetClient odieBotnetDevice( WIFI_SSID, WIFI_PASSWORD );
WebSocketsClient socket;

void eventHandler(WStype_t type, uint8_t* payload, size_t length) {
	switch(type) {
		case WStype_DISCONNECTED:
			break;
		case WStype_CONNECTED:
			break;
		case WStype_TEXT:
			Serial.println((char*) payload);
			break;
		case WStype_BIN:
			break;
	}
}

void setup() {
	Serial.begin( 115200 );
	while( !odieBotnetDevice.connect() ) {
		// Unable to connect to odieBotnet server
		Serial.println( "Unable to connect to OdieBotnet server. Errors: " );
		while(odieBotnetDevice.hasMoreErrors()) {
			Serial.print("\t");
			Serial.println(odieBotnetDevice.getNextError());
		}
		delay(200);
	}
	Serial.println( "OdieBotnet is connected." );

	socket = odieBotnetDevice.getSocket();
	socket.onEvent( eventHandler );
}

void loop() {
	socket.loop();
}
