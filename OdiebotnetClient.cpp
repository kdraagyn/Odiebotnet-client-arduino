/*
 	July 5, 2016
  	Implementation of OdiebotnetClient.h that is made to handle communication
 		and handle error handling with service discovery of the OdieBotnet server.
*/
#include "OdiebotnetClient.h"

OdiebotnetClient::OdiebotnetClient( char* ssid, char* password ) {
  this->ssid = ssid;
  this->password = password;
  this->setWifiCreds = true;
}

OdiebotnetClient::~OdiebotnetClient() { }

bool OdiebotnetClient::connect() {

  if ( !this->setWifiCreds ) {
    // #ifdef DEBUGGING
    Serial.println("OdieBotnet-Device must be created with wifi credentials");
    // #endif
    return false;
  }

  // #ifdef DEBUGGING
  // #endif

  // Connect to wifi network
  if ( !connectWifiNetwork( this->ssid, this->password ) ) {
    this->errorMessages.push("Unable to connect to Wifi");
    return false;
  }

  // #ifdef DEBUGGING
  Serial.print( "Connected to network (" );
  Serial.print(WiFi.localIP());
  Serial.println(")");

  Serial.println( "Attempting to find OdieBotnet server" );
  // #endif

  // Find Odiebotnet server on local network
  if ( !findOdieServer( &this->odieServerInfo ) ) {
    // Unable to find Odie server
    this->errorMessages.push("Unable to find OdieBotnet server");
    return false;
  }

  // #ifdef DEBUGGING
  Serial.print( "Found Odie Server at - " );
  Serial.print( odieServerInfo.address );
  Serial.print( ":" );
  Serial.println( odieServerInfo.port );

  Serial.println( "Attempting to connect to WebSocket" );
  // #endif

  // Connect through webSocket to OdieBotnet server
  if (!connectWebSocket( &this->odieServerInfo ) ) {
    this->errorMessages.push("Unable to connect to OdieBotnet server through websockes");
    return false;
  }

  // everything connected and OdieBotnet is ready to be controlled
  return true;
}

bool OdiebotnetClient::connectWifiNetwork(char* ssid, char* password) {
  // check if wifi has already been setup
  if( this->connectedWifi ) {
    return this->connectedWifi;
  }

  int tries = 0;

  WiFi.begin( ssid, password );
  do {
    delay( 500 );

    // #if DEBUGGING
    Serial.print('.');
    // #endif

    tries++;
    if ( tries > _WIFI_RETRIES ) {
      this->connectedWifi = false;
      this->errorMessages.push("Exceeded number of retries to connect to WiFi");
      return this->connectedWifi;
    }
    this->connectedWifi = ( WiFi.status() == WL_CONNECTED );
  } while ( !this->connectedWifi ) ;
  
  Serial.println();

  this->udp.begin(_DEVICE_UDP_LISTEN_PORT);

  return this->connectedWifi;
}

bool OdiebotnetClient::findOdieServer( OdieServerInfo* odieServerInfo ) {
  if( this->connectedUdp ) {
    return true;
  }
  
  // Send UDP broadcast message
  IPAddress broadcastAddress = calculateBroadcastAddress();
  Serial.print( "Calculated broadcast IPAddress - " );
  Serial.println( broadcastAddress );
  
  if ( !broadcastInfoUdp( broadcastAddress ) ) {
    this->errorMessages.push("Error occured in broadcasting UDP message to OdieBotnet server");
    return false;
  }

  // Wait for response
  this->connectedUdp = getDeviceId( odieServerInfo ) != _EMPTY_DEVICE_ID;
  return this->connectedUdp;
}

bool OdiebotnetClient::connectWebSocket( OdieServerInfo* odieInfo ) {
  char odieServerAddressBuffer[16];
  IPAddress odieAddress = odieInfo->address;
  uint16_t odiePort = odieInfo->port;

  sprintf( odieServerAddressBuffer, "%d.%d.%d.%d",
           odieAddress[0],
           odieAddress[1],
           odieAddress[2],
           odieAddress[3] );

  // Connect websocket with status response
  this->webSocket.begin(odieServerAddressBuffer, odiePort);
  return true;
}

bool OdiebotnetClient::broadcastInfoUdp( IPAddress ipaddress ) {
  char deviceInfoBuffer[256];
  sprintf(deviceInfoBuffer, "{\"id\":\"%d\", \"capabilities\":[]}", this->deviceId );

  IPAddress broadcastAddress = calculateBroadcastAddress();
  this->udp.beginPacket( broadcastAddress, _ODIE_UDP_BROADCAST_PORT );
  this->udp.write( deviceInfoBuffer );
  this->udp.endPacket();

  return true;
}

uint16_t OdiebotnetClient::getDeviceId( OdieServerInfo* serverInfo ) {
  int responseLength;

  // #if DEBUGGING
  Serial.println("Waiting for OdieResponse");
  // #endif

  int tries = 0;
  do {
    responseLength = this->udp.parsePacket();

    if( tries > _UDP_TIMEOUT ) {
      char errorMessage[100];
      sprintf( errorMessage, "OdieBotnet server didn't response within %d milliseconds", _UDP_TIMEOUT );
      this->errorMessages.enqueue( errorMessage );
      return _EMPTY_DEVICE_ID;
    }

    if(responseLength < 1) {
      delay(1);
      tries++;
    }
  } while ( responseLength < 1 );

  char* responseBody = new char[responseLength];
  udp.read( responseBody, responseLength );

  StaticJsonBuffer<200> responseJsonBuffer;
  JsonObject& responseRoot = responseJsonBuffer.parseObject( responseBody );

  if ( !responseRoot.success() ) {
    // #if DEBUGGING
    Serial.println( "Error parsing JSON response: " );
    responseRoot.prettyPrintTo( Serial );
    // #endif

    return _EMPTY_DEVICE_ID;
  }

  uint16_t id = responseRoot[ "id" ];
  serverInfo->port = responseRoot[ "port" ];
  serverInfo->address = udp.remoteIP();
  this->deviceId = id;

  Serial.print("Device id: ");
  Serial.println( this->getId() );

  return id;
}


IPAddress OdiebotnetClient::calculateBroadcastAddress() {
  return ~WiFi.subnetMask() | WiFi.gatewayIP();
}

void OdiebotnetClient::setId( uint16_t deviceId ) {
  this->deviceId = deviceId;
}

uint16_t OdiebotnetClient::getId() {
  return this->deviceId;
}

void OdiebotnetClient::setCapabilities( char** capabilities ) {
  this->deviceCapabilities = capabilities;
}

char** OdiebotnetClient::getCapabilities() {
  return this->deviceCapabilities;
}

char* OdiebotnetClient::getNextError() {
	return this->errorMessages.dequeue();
}

bool OdiebotnetClient::hasMoreErrors() {
	return !this->errorMessages.isEmpty();
}

WebSocketsClient OdiebotnetClient::getSocket() {
  return this->webSocket;
}
