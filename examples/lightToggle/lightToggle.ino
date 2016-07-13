#include "OdieBotnetDevice.h"
#include "OdieBotnetEnvironment.h"

#define DEBUGGING true

#define LIGHT_ID_TAG_NAME "lightId"
#define STATE_TAG_NAME "turnOn"
#define STATUS_TAG_NAME "isSuccessful"
#define MESSAGE_TAG_NAME "message"

#define LIGHT_COUNT 4

uint8_t lightIdToPin[] = { 0, 2, 4, 5 };
// lightId 0 -> pin 0
// lightId 1 -> pin 2
// lightId 2 -> pin 4
// lightId 3 -> pin 5

bool lightIdToState[] = {false, false, false, false};

OdieBotnetClient odieBotnet( WIFI_SSID, WIFI_PASSWORD );
WebSocketsClient socket;

// Queue for the responses that need to be sent back in the main loop rather than the eventHandler
QueueArray<char*> responseQueue;

// keep track if webSocket connected state has changed
bool webSocketConnected = false;

typedef struct {
	int lightId;
	bool state;
} PinData;

bool parseJsonToPinData( char* jsonString, PinData* pinData ) {
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& responseRoot = jsonBuffer.parseObject( jsonString );

	if( !responseRoot.success() ) {
		Serial.println( "[ERROR] Json Parse Failed");
		return false;
	}
	
	pinData->lightId = responseRoot[ LIGHT_ID_TAG_NAME ];
	pinData->state = responseRoot[ STATE_TAG_NAME ];

	return true;
}

char* generateToggleResponse( bool isSuccessful, char* message ) {
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& dataRoot = jsonBuffer.createObject();

	dataRoot[ STATUS_TAG_NAME ] = isSuccessful;
	dataRoot[ MESSAGE_TAG_NAME ] = message;

	char responseBuffer[ 100 ];
	dataRoot.printTo( responseBuffer, sizeof( responseBuffer ));

	return responseBuffer;
}

bool setLight( int lightId, bool state, char* message ) {
	// TODO: may need some validation done to add security 
	// check to make sure lightId is within the proper size
	if( lightId >= LIGHT_COUNT ) {
		message = "Unknown LightId";
		return false;
	}

	uint8_t pin = lightIdToPin[ lightId ];
	if( state ) { // turn light on
		digitalWrite( pin, HIGH );
		lightIdToState[ lightId ] = HIGH;
	} else {
		digitalWrite( pin, LOW );
		lightIdToState[ lightId ] = LOW;
	}
	return true;
}

void eventHandler(WStype_t type, uint8_t* payload, size_t length) {
	switch(type) {
		case WStype_DISCONNECTED:
			if(webSocketConnected ) {
				Serial.println( "Disconnected from OdieBotnet server.. :(" );
				webSocketConnected = false;
			}
			break;
		case WStype_CONNECTED:
			Serial.println( "Connected to OdieBotnet server! :)" );
			webSocketConnected = true;
			break;
		case WStype_TEXT:
			Serial.println( (char*) payload );

			bool isSuccessful;
			char* response;
			char* message;
			PinData pinData;

			// parse json to PinData object
			if( !parseJsonToPinData( (char*) payload, &pinData ) ) {
				isSuccessful = false;
				message = "Unable to parse request";
			} else {
				isSuccessful = setLight( pinData.lightId, pinData.state, message );
				if( isSuccessful ) {
					message = "";
				}
			}
			
			// generate and send response 
			response = generateToggleResponse( isSuccessful, message );
			responseQueue.push( response );
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
	Serial.println( "Connected to OdieBotnet webSocket!" );

	// Set all light controlling output types
	for( int c = 0; c < LIGHT_COUNT; c++ ) {
		pinMode( lightIdToPin[ c ], OUTPUT );
	}

	socket = odieBotnet.getSocket();
	socket.onEvent( eventHandler );
}

void loop() {
	socket.loop();
	if( !responseQueue.isEmpty() ) {
		char* response = responseQueue.pop();
		Serial.println( response );
		socket.sendTXT( response );
	}
}
