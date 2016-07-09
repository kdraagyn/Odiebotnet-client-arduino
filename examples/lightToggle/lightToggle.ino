#include "OdieBotnetDevice.h"
#include "OdieBotnetEnvironment.h"

#define DEBUGGING true

#define LIGHT_ID_TAG_NAME "lightId"
#define STATE_TAG_NAME "state"
#define STATUS_TAG_NAME "status"

#define LIGHT_COUNT 2
uint8_t lightIdToPin[] = { 12, 13 };
// lightId 0 -> 12
// lightId 1 -> 13

OdieBotnetClient odieBotnet( WIFI_SSID, WIFI_PASSWORD );
WebSocketsClient socket;

// Used to parse json queries and generate json responses
StaticJsonBuffer<200> jsonBuffer;

typedef struct {
	int lightId;
	bool state;
} PinData;

PinData parseJsonToPinData( char* jsonString ) {
	JsonObject& responseRoot = jsonBuffer.parseObject( jsonString );
	
	PinData pinData;
	pinData.lightId = responseRoot[ LIGHT_ID_TAG_NAME ];
	pinData.state = responseRoot[ STATE_TAG_NAME ];

	return pinData;
}

char* generateToggleResponse( bool isSuccessful ) {
	JsonObject& dataRoot = jsonBuffer.createObject();

	dataRoot[ STATUS_TAG_NAME ] = isSuccessful;

	char jsonBuffer[ 100 ];
	dataRoot.printTo( jsonBuffer, sizeof( jsonBuffer ));

	return jsonBuffer;
}

bool setLight( int lightId, bool state ) {
	// TODO: may need some validation done to add security 
	uint8_t pin = lightIdToPin[ lightId ];
	if( state ) { // turn light on
		digitalWrite( pin, HIGH );
	} else {
		digitalWrite( pin, LOW );
	}
}

void eventHandler(WStype_t type, uint8_t* payload, size_t length) {
	switch(type) {
		case WStype_DISCONNECTED:
			break;
		case WStype_CONNECTED:
			break;
		case WStype_TEXT:

			bool isSuccessful;
			char* response;
			PinData pinData;

			// parse json to PinData object
			pinData = parseJsonToPinData( (char*) payload );
			isSuccessful = setLight( pinData.lightId, pinData.state );
			
			// generate and send response 
			response = generateToggleResponse( isSuccessful );
			socket.sendTXT( response );

			break;
		case WStype_BIN:
			break;
	}
}

void setup() {
	Serial.begin( 115200 );
	while( !odieBotnet.connect() ) {
		Serial.println( "Unable to connect to OdieBotnet server. Errors: " );

		while( odieBotnet.hasMoreErrors() ) {
			Serial.print( "\t" );
			Serial.println( odieBotnet.getNextError() );
		}

		delay(200);
	}

	// Set all light controlling output types
	for( int c = 0; c < LIGHT_COUNT; c++ ) {
		pinMode( lightIdToPin[ c ], OUTPUT );
	}
}

void loop() {
	socket.loop();
}
