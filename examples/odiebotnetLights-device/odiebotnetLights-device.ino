/*
 *	ESP 8266 sketch for controlling a light through
 *		the OdieBotnet environment.
 *
 *	Author: Keith Nygaard
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

#include "OdiebotnetClient.h"
#include "OdiebotnetConfig.h"

#define HOST_NAME "odieBotnet-device-1"
#define HOST_PORT 80

#define LIGHT_ID_TAG_NAME "lightId"
#define STATE_TAG_NAME "turnOn"
#define STATUS_TAG_NAME "isSuccessful"
#define MESSAGE_TAG_NAME "message"

#define LIGHT_COUNT 2
uint8_t lightIdToPin[] = { 5, 12 };
bool lightIdToState[] = { false, false };

OdiebotnetClient odieBotnet = OdiebotnetClient( WIFI_SSID, WIFI_PASSWORD );
WebSocketsClient socket;

ESP8266WebServer server( HOST_PORT );

QueueArray<char*> webSocketResponseQueue;

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
		Serial.println( "[ERROR] Json Parse Failed :(" );
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
	dataRoot.printTo( responseBuffer, sizeof( responseBuffer ) );

	return responseBuffer;
}

bool setLight( int lightId, bool state, char* message ) {
	if( lightId >= LIGHT_COUNT ) {
		message = "Unknown LightId";
		return false;
	}

	uint8_t pin = lightIdToPin[ lightId ];
	if ( state ) { // turn light on
		digitalWrite( pin, HIGH );
	} else {
		digitalWrite( pin, LOW );
	}
	lightIdToState[ lightId ] = state;

	return true;
}

void eventHandler( WStype_t type, uint8_t* payload, size_t length ) {
	switch ( type ) {
		case WStype_DISCONNECTED:
			if( webSocketConnected ) {
				Serial.println( "Disconnected from OdieBotnet server... :(" );
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
				Serial.printf("Light ID: %d\n", pinData.lightId );
				isSuccessful = setLight( pinData.lightId, pinData.state, message );
				if ( isSuccessful ) {
					message = "";
				}
			}

			// generate and send response
			response = generateToggleResponse( isSuccessful, message );
			webSocketResponseQueue.push( response );
			break;
		case WStype_BIN:
			Serial.println( "Unable to handle binary data" );
			char* binaryResponse;
			binaryResponse = "{\"status\":\"FAIL\", \"message\":\"Unable to handle binary data\"}";
			webSocketResponseQueue.push( binaryResponse );
			break;
	}
} 

void setup() {
	Serial.begin( 115200 );

	while( !odieBotnet.connect() ) {
		Serial.println( "Unable to connect to OdieBotnet, Trying again.. ");

		while( odieBotnet.hasMoreErrors() ) {
			Serial.print( "\t" );
			Serial.println( odieBotnet.getNextError() );
		}

		delay( 200 );
	}

	// Device has connected to OdieBotnet server
	socket = odieBotnet.getSocket();
	socket.onEvent( eventHandler );

	// create server for updating sketch OTA
	server.on( "/update", HTTP_POST, []() {
		server.sendHeader( "Connection", "close" );
		server.sendHeader( "Access-Control-Allow-Origin", "*" );
		
		if ( !Update.hasError() ) {
			server.send( 200, "application/json", "{\"status\":\"OK\"}" );
			ESP.restart();
		} else {
			server.send( 200, "application/json", "{\"status\":\"FAIL\"}" );
		}
	}, []() {
		HTTPUpload& upload = server.upload();
		if ( upload.status == UPLOAD_FILE_START ) {
			WiFiUDP::stopAll();
			Serial.printf( "Update: %s\n", upload.filename.c_str() );
			uint32_t maxSketchSpace = ( ESP.getFreeSketchSpace() - 0x1000 ) & 0xFFFFF000;
			if ( !Update.begin( maxSketchSpace ) ) {
				Update.printError( Serial );
			}
		} else if ( upload.status == UPLOAD_FILE_WRITE ) {
			if( Update.write( upload.buf, upload.currentSize ) != upload.currentSize ) {
				Update.printError( Serial );
			}
		} else if ( upload.status == UPLOAD_FILE_END ) {
			if ( Update.end( true ) ) {
				Serial.printf("Update Success: %u\n", upload.totalSize );
			} else {
				Update.printError( Serial );
			}
		}
		yield();
	});

	server.on( "/setNetwork", HTTP_GET, []() {
		int argsCount = server.args();
		if ( argsCount == 2 ) {
			char* messageBack = new char[100];
			sprintf( messageBack, "{\"status\":\"FAIL\", \"reason\":\"Unimplemented\", \"ssid\":\"%s\", \"password\":\"%s\"}", server.arg("ssid").c_str(), server.arg("password").c_str() );
			server.sendHeader( "Connection", "close" );
			server.sendHeader( "Access-Control-Allow-Origin", "*" );
			server.send( 200, "application/json", messageBack );
		}
	});

	server.begin();
	MDNS.addService( "http", "tcp", HOST_PORT );
	Serial.printf( "Listening to http://%s.local on your network\n", HOST_NAME );

	// set light pins to output
	for ( int c = 0; c < LIGHT_COUNT; c++ ) {
		pinMode( lightIdToPin[ c ], OUTPUT );
	}
}

void loop() {
	socket.loop();
	server.handleClient();

	if( !webSocketResponseQueue.isEmpty() ) {
		char* response = webSocketResponseQueue.pop();
		Serial.println( response );
		socket.sendTXT( response );
	}

	// help with server.handleClientTiming
	delay( 1 );
}
