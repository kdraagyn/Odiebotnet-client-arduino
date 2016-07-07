/*
 	July 5, 2016
  	Implementation of OdieBotnetClient.h that is made to handle communication
 		and handle error handling with service discovery of the OdieBotnet server.
*/
#include "OdieBotnetDevice.h"

OdieBotnetClient::OdieBotnetClient( char* ssid, char* password ) {
  this->ssid = ssid;
  this->password = password;
  this->setWifiCreds = true;
}

OdieBotnetClient::~OdieBotnetClient() {
}

bool OdieBotnetClient::connect() {

  if ( !this->setWifiCreds ) {
    // #ifdef DEBUGGING
    Serial.println("OdieBotnet-Device must be created with wifi credentials");
    // #endif
    // hasn't set wifi creds
    return false;
  }

  // #ifdef DEBUGGING
  Serial.print("Attempting to connect - ");
  Serial.println(ssid);
  // #endif

  // Connect to wifi network
  if ( !connectWifiNetwork( this->ssid, this->password ) ) {
    // Unable to connect to network
    return false;
  }

  // #ifdef DEBUGGING
  Serial.println( "Connected to network" );
  Serial.println( "Attempting to find OdieBotnet server" );
  // #endif

  // Find Odiebotnet server on local network
  if ( !findOdieServer( &this->odieServerInfo ) ) {
    // Unable to find Odie server
    Serial.println("Unable to find OdieBotnetServer");
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
    // Unable to connect to socket
    return false;
  }

  // everything connected and OdieBotnet is ready to be controlled
  return true;
}

bool OdieBotnetClient::connectWifiNetwork(char* ssid, char* password) {
  int tries = 0;

  do {
    delay( 500 );

    // #if DEBUGGING
    Serial.print('.');
    // #endif

    tries++;
    if ( tries > 30 ) {
      // #if DEBUGGING
      Serial.println();
      Serial.println("Exceeded number of retries (30)");
      // #endif
      this->connectedWifi = false;
      return this->connectedWifi;
    }
    this->connectedWifi = ( WiFi.status() == WL_CONNECTED );
  } while ( !this->connectedWifi ) ;
  
  /// #if DEBUGGING
  Serial.println();
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.print(_DEVICE_UDP_LISTEN_PORT);
  Serial.println();
  // #endif

  this->udp.begin(_DEVICE_UDP_LISTEN_PORT);

  return this->connectedWifi;
}

bool OdieBotnetClient::findOdieServer( OdieServerInfo* odieServerInfo ) {
  if( this->connectedUdp ) {
    return true;
  }
  
  // Send UDP broadcast message
  IPAddress broadcastAddress = calculateBroadcastAddress();
  Serial.print( "Calculated broadcast IPAddress - " );
  Serial.println( broadcastAddress );
  
  if ( !broadcastInfoUdp( broadcastAddress ) ) {
    // failed to send broadcast UDP
    return false;
  }

  // Wait for response
  this->connectedUdp = ( getDeviceId( odieServerInfo ) != -1 );
  return this->connectedUdp;
}

bool OdieBotnetClient::connectWebSocket( OdieServerInfo* odieInfo ) {
  char* odieServerAddressBuffer = new char[16];
  IPAddress odieAddress = odieInfo->address;
  uint16_t odiePort = odieInfo->port;

  sprintf( odieServerAddressBuffer, "%d.%d.%d.%d",
           odieAddress[0],
           odieAddress[1],
           odieAddress[2],
           odieAddress[3] );

  // Connect websocket with status response
  this->webSocket.begin(odieServerAddressBuffer, odiePort);
}

WebSocketsClient OdieBotnetClient::getSocket() {
  return this->webSocket;
}

bool OdieBotnetClient::broadcastInfoUdp( IPAddress ipaddress ) {
  char deviceInfoBuffer[256];
  sprintf(deviceInfoBuffer, "{\"id\":\"%d\", \"capabilities\":[]}", this->deviceId );

  IPAddress broadcastAddress = calculateBroadcastAddress();
  this->udp.beginPacket( broadcastAddress, _ODIE_UDP_BROADCAST_PORT );
  this->udp.write( deviceInfoBuffer );
  this->udp.endPacket();
}

uint16_t OdieBotnetClient::getDeviceId( OdieServerInfo* serverInfo ) {
  int responseLength;

  // #if DEBUGGING
  Serial.print("Waiting for OdieResponse");
  // #endif

  do {
    responseLength = this->udp.parsePacket();
  } while ( responseLength < 1 );

  // #if DEBUGGING
  Serial.println( "Done" );
  // #endif

  char* responseBody = new char[responseLength];
  udp.read( responseBody, responseLength );

  Serial.println( responseLength );
  Serial.println( responseBody );
  
  StaticJsonBuffer<200> responseJsonBuffer;
  JsonObject& responseRoot = responseJsonBuffer.parseObject( responseBody );

  if ( !responseRoot.success() ) {
    // #if DEBUGGING
    Serial.println( "Error parsing JSON response: " );
    responseRoot.prettyPrintTo( Serial );
    // #endif

    return -1;
  }

  uint16_t id = responseRoot[ "id" ];
  serverInfo->port = responseRoot[ "port" ];
  serverInfo->address = udp.remoteIP();
  this->deviceId = id;

  return id;
}


IPAddress OdieBotnetClient::calculateBroadcastAddress() {
  return ~WiFi.subnetMask() | WiFi.gatewayIP();
}

void OdieBotnetClient::setId( uint16_t deviceId ) {
  this->deviceId = deviceId;
}

uint16_t OdieBotnetClient::getId() {
  return this->deviceId;
}

void OdieBotnetClient::setCapabilities( char** capabilities ) {
  this->deviceCapabilities = capabilities;
}

char** OdieBotnetClient::getCapabilities() {
  return this->deviceCapabilities;
}
