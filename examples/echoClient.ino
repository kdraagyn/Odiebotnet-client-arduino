#include "OdieBotnetDevice.h"

// Used to give meaningful serial output
//	Serial MUST be initialized before odieBotnet is connected or used
#define DEBUGGING true

const char* ssid = "OhDisWiFi";
const char* password = "underwaterhorsesarecalledseahorses";

OdieBotnetClient odieBotnetDevice( ssid, password );

void setup() {
	Serial.begin( 115200 );
	if( !odieBotnetDevice.connect() ) {
		// Unable to connect to odieBotnet server
		Serial.println( "Unable to connect to OdieBotnet server" );
		while( 1 ); // loop forever
	}
	Serial.println( "OdieBotnet is connected." );
}

void loop() {
	// send heart beats every couple seconds
	odieBotnetDevice.send( "Pew Pew Pew....." );
	delay( 2000 );
}
