/**************************************************************************/
/*!
    @file     DMX2WiFi.ino
    @author   Claude Heintz
    @license  BSD (see LXESP8266DMX LICENSE)
    @copyright 2015 by Claude Heintz

    Example using LXEDMXWiFi_Library for input from DMX to Art-Net or E1.31 sACN
    sent via ESP 8266 WiFi connection
    
    Art-Net(TM) Designed by and Copyright Artistic Licence (UK) Ltd
    sACN E 1.31 is a public standard published by the PLASA technical standards program
    
    Note:  For sending packets larger than 512 bytes, ESP8266 WiFi Library v2.1
           is necessary.
           
           This example requires the LXESP8266DMX library for DMX serial output
           https://github.com/claudeheintz/LXESP8266DMX
    
    @section  HISTORY

    v1.0 - First release
*/
/**************************************************************************/
#include <LXESP8266UARTDMX.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <LXDMXWiFi.h>
#include <LXArtNet.h>
#include <LXSACN.h>

const char* ssid = "ESP2DMX";
const char* pwd = 0;
uint8_t make_access_point = 0;
uint8_t chan = 2;

uint8_t use_sacn = 1;
uint8_t use_multicast = 1;

// dmx protocol interface for parsing packets (created in setup)
LXDMXWiFi* interface;

// LX8266DMXInput instance
LX8266DMXInput* dmx_input = new LX8266DMXInput();

// An EthernetUDP instance to let us send and receive UDP packets
WiFiUDP wUDP;

uint8_t led_state = 0;

void blinkLED() {
  if ( led_state ) {
    digitalWrite(BUILTIN_LED, HIGH);
    led_state = 0;
  } else {
    digitalWrite(BUILTIN_LED, LOW);
    led_state = 1;
  }
}

int got_dmx = 0;
void gotDMXCallback(int slots);
IPAddress send_address;

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  
  if ( make_access_point ) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
    WiFi.softAPConfig(IPAddress(10,110,115,10), IPAddress(10,1,1,1), IPAddress(255,0,0,0));
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid,pwd);
    WiFi.config(IPAddress(10,110,115,20), IPAddress(192,168,1,1), IPAddress(255,0,0,0));

    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      blinkLED();
    }
  }

  if ( use_sacn ) {                  // Initialize Interface
    interface = new LXSACN();
  } else {
    interface = new LXArtNet(WiFi.localIP(), WiFi.subnetMask());
    use_multicast = 0;
  }

  if ( use_multicast ) {  // *not necessary to start listening for UDP on port
    /*if ( make_access_point ) {
      wUDP.beginMulticast(WiFi.softAPIP(), IPAddress(239,255,0,1), interface->dmxPort());
    } else {
      wUDP.beginMulticast(WiFi.localIP(), IPAddress(239,255,0,1), interface->dmxPort());
    }*/
    send_address = IPAddress(239,255,0,1);
  } else {
    //wUDP.begin(interface->dmxPort());
    send_address = IPAddress(10,255,255,255);   // change if unicast is desired
  }

  dmx_input->setDataReceivedCallback(&gotDMXCallback);
  dmx_input->start();

  //note requires v2.1 of ESP8266WiFi library for >494 slots Art-Net
  //prior to fix, total packet size is limited to 512 bytes
  interface->setNumberOfSlots(512);
}


// ***************** input callback function *************

void gotDMXCallback(int slots) {
  got_dmx = slots;
}

/************************************************************************

  The main loop fades the levels of addresses 7 and 8 to full
  
*************************************************************************/

void loop() {
  if ( got_dmx ) {
    //interface->setNumberOfSlots(got_dmx);
    for(int i=1; i<=got_dmx; i++) {
      interface->setSlot(i, dmx_input->getSlot(i));
    }
    if ( use_multicast ) {
       if ( make_access_point ) {
          interface->sendDMX(wUDP, send_address, WiFi.softAPIP());
       } else {
          interface->sendDMX(wUDP, send_address, WiFi.localIP());
       }
    } else {
       interface->sendDMX(wUDP, send_address, INADDR_NONE);
    }
    got_dmx = 0;
    blinkLED();
  } //got_dmx
}
