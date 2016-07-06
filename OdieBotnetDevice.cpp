/*
 *	July 5, 2016
 * 	Implementation of OdieBotnetClient.h that is made to handle communication
 *		and handle error handling with service discovery of the OdieBotnet server.
 */
#include "OdieBotnetClient.h"

OdieBotnetClient::OdieBotnetClient( char* ssid, char* password ) {
	StaticJsonBuffer<200> jsonBuffer;
	this.deviceInfo = jsonBuffer.createObject();

	this.ssid = ssid;
	this.password = password;
	this.setWifiCreds = true;
}

OdieBotnetClient::~OdieBotnetClient() {
	delete this.deviceInfo;
}

bool OdieBotnetClient::connect() {

	if( !this->setWifiCreds ) {
#if DEBUGGIN
		Serial.println("OdieBotnet-Device must be created with wifi credentials");
#endif
		// hasn't set wifi creds
		return false;
	}

#if DEBUGGING
	Serial.print("Attempting to connect - ");
	Serial.println(ssid);
#endif

	// Connect to wifi network
	if( !connectWifiNetwork( this.ssid, this.password ) ) {
		// Unable to connect to network
		return false;
	}

#if DEBUGGING
	Serial.println( "Connected to network" );
	Serial.println( "Attempting to find OdieBotnet server" );
#endif

	OdieServerInfo odieServerInfo;

	// Find Odiebotnet server on local network
	if( !findOdieServer( odieServerInfo& ) ) {
		// Unable to find Odie server
		return false;
	}

#if DEBUGGING
	Serial.print( "Found Odie Server at - " );
	Serial.print( odieServerinfo.address );
	Serial.print( ":" );
	Serial.println( odieServerInfo.port );

	Serial.println( "Attempting to connect to WebSocket" );
#endif

	// Connect through webSocket to OdieBotnet server
	if(!connectWebSocket( odieServerInfo )) {
		// Unable to connect to socket
		return false;
	}

	// everything connected and OdieBotnet is ready to be controlled
	return true;
}

bool OdieBotnetClient::connectWifiNetwork(char* ssid, char* password) {
	int tries = 0;
	bool connected = true;

	while( Wifi.status() != WL_CONNECTED) ) {
		delay( 500 );

#if DEBUGGING
		Serial.print('.');
#endif

		tries++;
		if( tries > 30 ) {
#if DEBUGGING
			Serial.println();
			Serial.println("Exceeded number of retries (30)");
#endif
			connected = false;
			break;
		}

#if DEBUGGING
		Serial.println();
#endif
	}

	return connected;
}

bool OdieBotnetClient::findOdieServer( OdieServerInfo* odieInfo ) {
	// Send UDP broadcast message
	IPAddress broadcastAddress = calculateBroadcastAddress();
	if( !broadcastInfoUdp( broadcastAddress ) ) {
		// failed to send broadcast UDP
		return false;
	}

	// wait 100 ms for response
	delay( 100 );

	// Wait for response
	if( getDeviceId( odieServerInfo ) != -1 ) {
		// Something went wrong in getting device ID
		return false;
	}

	return true;
}

bool OdieBotnetClient::connectWebSocket( OdieServerInfo* odieInfo ) {
	char* odieServerAddressBuffer[16];
	IPAddress odieAddress = odieInfo->address;
	uint16_t odiePort = odieInfo->port;

	sprintf( odieServerAddressBuffer, "%d.%d.%d.%d", 
		odieAddress[0], 
		odieAddress[1], 
		odieAddress[2], 
		odieAddress[3] );

	// Connect websocket with status response
	if( !this.client.connect( odieServerAddressBuffer, odiePort) {

#if DEBUGGING
		Serial.println("Unable to connect to WebSocket");
#endif
		return false;
	}

	this.webSocketClient.path = "/";
	this.webSocketClient.host = odieServerAddressBuffer;
	
	if( !this.webSocketClient.handshake( client ) ) {
		// Unable to connect web socket

#if DEBUGGING
		Serial.println("Unable to complete handshake for WebSocket");
#endif

		return false;
	}
	
	return true;
}

bool OdieBotnetClient::broadcastInfo( IPAddress ipaddress ) {
	char deviceInfoBuffer[256];
	this.deviceInfo.printTo( deviceInfoBuffer, sizeof( deviceInfoBuffer ) );

	Udp.beginPacket( broadcastAddress, _ODIE_UDP_BROADCAST_PORT );
	Udp.write( deviceInfoBuffer );
	Udp.endPacket();
}

uint16_t OdieBotnetClient::getDeviceId( OdieServerInfo* serverInfo ) {
	int reponseLength;

#if DEBUGGING
	Serial.print("Waiting for OdieResponse");
#endif

	do {
		responseLength = Udp.parsePacket();
		delay( 100 );

#if DEBUGGING
		Serial.print('.');
#endif
	} while( responseLength < 1 );

#if DEBUGGING
	Serial.println( "Done" );
#endif

	char* responseBody = new char[responseLength];
	Udp.read( responseBody, responseLength );

	StaticJsonBuffer<200> responseJsonBuffer;
	JsonObject& responseRoot = responseJsonBuffer.parseObject( responseBody );

	if( !responseRoot.success() ) {
#if DEBUGGING
		Serial.println( "Error parsing JSON response: " );
		responseRoot.prettyPrintTo( Serial );
#endif

		return -1;
	}
	
	uint16_t id = responseRoot[ "id" ];
	serverInfo->port = responseRoot[ "port" ];
	serverInfo->address = Udp.remoteIP();

	return id;
}
	

IPAddress OdieBotnetClient::calculateBroadcastAddress() {
	return ~Wifi.subnet_mask() | wifi.gatewayIp();
}

void OdieBotnetClient::setId( uint16_t deviceId ) {
	this.deviceInfo[ _DEVICE_ID_KEY ] = deviceId;
}

uint16_t OdieBotnetClint::getId() {
	return this.deviceInfo[ _DEVICE_ID_KEY ];
}

void setCapabilities( char** capabilities ) {
	this.deviceInfo[ _DEVICE_CAPABILITIES_KEY ] = capabilities;
}

char** getCapabilities() {
	return this.deviceInfo[ _DEVICE_CAPABILITIES_KEY ];
}
