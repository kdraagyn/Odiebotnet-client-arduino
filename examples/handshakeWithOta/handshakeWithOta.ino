#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include "OdieBotnetDevice.h"
#include "OdieBotnetEnvironment.h"

#define HOST_NAME "odieBotnet-device"
#define HOST_PORT 80

OdieBotnetClient odieBotnet = OdieBotnetClient( WIFI_SSID, WIFI_PASSWORD );
WebSocketsClient socket;

ESP8266WebServer server( HOST_PORT );

void eventHandler( WStype_t type, uint8_t* payload, size_t lenght ) {
	switch ( type ) {
		case WStype_DISCONNECTED:
			break;
		case WStype_CONNECTED:
			break;
		case WStype_TEXT:
			break;
		case WStype_BIN:
			break;
	}
}

void setup() {
	Serial.begin( 115200 );

	while( !odieBotnet.connect() ) {
		Serial.println("Unable to connect to OdieBotnet, Trying again..");
	
		while( odieBotnet.hasMoreErrors() ) {
			Serial.print( "\t" );
			Serial.println( odieBotnet.getNextError() );
		}

		delay( 200 );
	}

	socket = odieBotnet.getSocket();
	socket.onEvent( eventHandler );

	// create MDNS option to allow for updating
	MDNS.begin( HOST_NAME );

	// create server for updating 
	server.on( "/update", HTTP_POST, []() {
		server.sendHeader( "Connection", "close" );
		server.sendHeader( "Access-Control-Allow-Origin", "*" );
		server.send(200, "application/json", ( Update.hasError() )?"{\"status\":\"FAIL\"}":"{\"status\":\"OK\"}" );
		ESP.restart();
	}, []() {
		HTTPUpload& upload = server.upload();
		if( upload.status == UPLOAD_FILE_START ) {
			WiFiUDP::stopAll();
			Serial.printf("Update: %s\n", upload.filename.c_str() );
			uint32_t maxSketchSpace = ( ESP.getFreeSketchSpace() - 0x1000 ) & 0xFFFFF000;
			if( !Update.begin(maxSketchSpace ) ) {
				Update.printError( Serial );	
			}
		} else if ( upload.status == UPLOAD_FILE_WRITE ) {
			if( Update.write( upload.buf, upload.currentSize ) != upload.currentSize ) {
				Update.printError( Serial );
			}
		} else if ( upload.status == UPLOAD_FILE_END ) {
			if( Update.end( true ) ) {
				Serial.printf("Update Success: %u\n", upload.totalSize );
			} else {
				Update.printError( Serial );
			}
		}
		yield();
	});

	server.on( "/setNetwork", HTTP_GET, []() {
		// print out request json
		int argsCount = server.args();
		Serial.printf( "SSID: %s \nPASSWORD: %s \n", server.arg("ssid").c_str(), server.arg("password").c_str() );
		server.sendHeader( "Connection", "close" );
		server.sendHeader( "Access-Control-Allow-Origin", "*" );
		server.send( 200, "application/json", "{\"status\":\"FAIL\", \"reason\":\"Unimplemented\"}" );
	});

	server.begin();
	MDNS.addService("http", "tcp", HOST_PORT );
	Serial.printf( "Listening to http://%s.local on your network\n", HOST_NAME );
}

void loop() {
	socket.loop();
	server.handleClient();

	// help with server.handleClient timing
	delay(1);
}

