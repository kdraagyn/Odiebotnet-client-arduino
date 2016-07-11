# OdieBotnet-Device

This Library is meant to allow for easy access to OdieBotnet device services. Odiebotnet manages an automatically configurable home automation service that new devices should use in order to be controlled and be used in the OdieBotnet ecosystem.

In order to connect to a router a file called "OdieBotnetEnvironment.h" must be written that includes definitions for WIFI\_SSID and WIFI\_PASSWORD.

To update over the air (OTA) using a POST request to the device use a syntax similar to this curl:
	curl -F upload=@"/tmp/buildf758efe33e64e02e147ea7e3dbd0bb65.tmp/handshakeWithOta.ino.bin" -H "Content-Type: multipart/form-data" 192.168.0.184/update

	The "-F" switch is important as it designates a file to be loaded to the esp8266 and the new sketch to be loaded to the board.

To set the wifi credentials us a GET request to the device using the syntax similar to this curl:
	curl -X GET 192.168.0.184/setNetwork?ssid=****&password=****
