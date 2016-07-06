/*
 *	July 5, 2016
 *	This library creates a OdieBotnet client that connects to the OdieBotnet 
 *		system.
 */

#ifndef ODIEBOTNET_DEVICE_H_
#define ODIEBOTNET_DEVICE_H_

#include <WifiUDP.h>
#include <ESP8266WiFi.h>

// TODO: pull udp broadcast port from config/eeprom
#define _ODIE_UDP_BROADCAST_PORT 8080

#define _DEVICE_ID_KEY "deviceInfo"
#define _DEVICE_CAPABILITIES_KEY "deviceCapabilities"

typedef struct OdieServerInfo {
	IPAddress address;
	uint16_t port;
} OdieServerInfo;

class OdieBotnetClient {
public:
	// Connect to OdieBotnet server instance on Local Network
	//	0. Connect to wifi network
	//	1. Broadcast UDP to find server
	//	2. Recieve response with assigned ID from OdieBotnet Server
	//	3. Connect webSocketClient to OdieBotnet server
	bool connect();

	void setId( uint16_t );
	uint16_t getId();
	void setCapabilities( char** );
	char** getCapabilities();

	OdieBotnetClient( char*, char* );
	~OdieBotnetClient();

private:
	// Connect ESP8266 to wifi network
	bool connectWifiNetwork( char*, char* );

	// Attempt to locate OdieBotnet Server
	bool findOdieServer( OdieServerInfo* );
	bool broadcastInfoUdp( IPAddress );
	uint16_t getDeviceId( OdieServerInfo* );

	// Connect to OdieBotnet server through webSocket
	bool connectWebSocket( OdieServerInfo* );

	// Calculate the correct broadcast address for broadcast UDP
	IPAddress calculateBroadcastAddress();

	// Model that holds the device's metadata
	JsonObject* deviceInfo;

	// Maintain state for connecting to wifi
	char* ssid;
	char* password;

	// Maintain state for communication through web socket
	WebSocketClient webSocketClient;
	WifiClient wifiClient;
}
