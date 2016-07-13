#include "OdiebotnetClient.h"
#include "OdiebotnetConfig.h"

#define LOOP_COUNT_CUTOFF 100000
#define TOGGLE_PIN 12

OdiebotnetClient odieBotnet = OdiebotnetClient( WIFI_SSID, WIFI_PASSWORD );
WebSocketsClient socket;

bool builtInLedState = false;
int loopCount = 0;

void eventHandler( WStype_t type, uint8_t* payload, size_t length ) {
	// handle message
	switch( type) {
		case WStype_DISCONNECTED:
			{
				// update status LED to orange
			}
			break;
		case WStype_CONNECTED:
			{
				// update status LED to green
				Serial.println("Connected to web socket!");
			}
			break;
		case WStype_TEXT:
			Serial.println((char *) payload);
			if(builtInLedState) {
				digitalWrite( TOGGLE_PIN, HIGH );
				Serial.println( "Turning on LED" );
			} else {
				digitalWrite( TOGGLE_PIN, LOW );
				Serial.println( "Turning Off LED" );
			}
			builtInLedState = !builtInLedState;
			break;
		case WStype_BIN:
			break;
	}
}

void setup() {
	Serial.begin(115200);
	pinMode( TOGGLE_PIN, OUTPUT );

	// connect to odieBotnet
	while(!odieBotnet.connect()) {
		delay(200);
	}

	socket = odieBotnet.getSocket();
	socket.onEvent( eventHandler );
}

void loop() {
	// wait for commands
	socket.loop();
	if( loopCount++ > LOOP_COUNT_CUTOFF ) {
		char* message = new char[500];
		sprintf( message, "{\"id\":%d,\"message\":\"Pew Pew Pew..\"}", odieBotnet.getId());
		socket.sendTXT( message );
		loopCount = 0;
	};
}
