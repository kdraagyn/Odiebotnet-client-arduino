/*
 *	July 5, 2016
 *	This library creates a Odiebotnet client that connects to the Odiebotnet 
 *		system.
 */

#ifndef ODIEBOTNET_CLIENT_H_
#define ODIEBOTNET_CLIENT_H_

#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <QueueArray.h>
#include <string.h>

// TODO: pull udp broadcast port from config/eeprom
#define _ODIE_UDP_BROADCAST_PORT 8080
#define _DEVICE_UDP_LISTEN_PORT 9090

#define _DEVICE_ID_KEY "deviceId"
#define _DEVICE_CAPABILITIES_KEY "deviceCapabilities"

#define _EMPTY_DEVICE_ID 0
#define _UDP_TIMEOUT 2000
#define _WIFI_RETRIES 30

typedef struct OdieServerInfo {
	IPAddress address;
	uint16_t port;
} OdieServerInfo;

class OdiebotnetClient {
public:
	// Connect to Odiebotnet server instance on Local Network
	//	0. Connect to wifi network
	//	1. Broadcast UDP to find server
	//	2. Recieve response with assigned ID from Odiebotnet Server
	//	3. Connect webSocketClient to Odiebotnet server
	bool connect();

	void setId( uint16_t );
	uint16_t getId();
	void setCapabilities( char** );
	char** getCapabilities();
	char* getNextError();
	bool hasMoreErrors();

	// Create Odiebotnet object with the credentials to connect to wifi
	// 	Only sets up OdiebotnetDevice to connect. Does not try to 
	//	connect to an Odiebotnet server.
	OdiebotnetClient( char*, char* );
	~OdiebotnetClient();

	// expose websocket client
	WebSocketsClient getSocket();
private:
	// Connect ESP8266 to wifi network
	bool connectWifiNetwork( char*, char* );

	// Attempt to locate Odiebotnet Server
	bool findOdieServer( OdieServerInfo* );
	bool broadcastInfoUdp( IPAddress );
	uint16_t getDeviceId( OdieServerInfo* );

	// Connect to Odiebotnet server through webSocket
	bool connectWebSocket( OdieServerInfo* );

	// Calculate the correct broadcast address for broadcast UDP
	IPAddress calculateBroadcastAddress();

	// Errors during operation
	QueueArray<char*> errorMessages;

	// Maintain state for connecting to wifi
	char* ssid;
	char* password;
	bool setWifiCreds = false;
	WiFiUDP udp;

	uint16_t deviceId;
	char** deviceCapabilities;

	// Maintain state of connections
	bool connectedWifi = false;
	bool connectedUdp = false;
	bool connectedSocket = false;
	OdieServerInfo odieServerInfo;

	// Maintain state for communication through web socket
	WebSocketsClient webSocket;
	WiFiClient wifiClient;
};

#endif
