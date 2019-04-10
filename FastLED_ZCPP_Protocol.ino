/*
* ARDUINO Arduino ESP8266 and ESP32 FASTLED SKETCH
* BETA VERSION 1.00
* DARREN HEDLUND
* USING the Official ZCPP Protocol BY Keith Westley
*
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/
#define FASTLED_SHOW_CORE 0
#define FASTLED_ESP32_RAW_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0
#include "ESPAsyncZCPP.h"
#include <FastLED.h>
FASTLED_USING_NAMESPACE

// FASTLED SETUP
#define MILLI_AMPS  		60000 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define VOLTS				5
uint8_t BRIGHTNESSINDEX = 	20;

#define NUM_LEDS			256
#define DATA_PIN    		3
#define LED_TYPE    		WS2812B
#define COLOR_ORDER 		GRB

// DEFINE THE LEDS PER PIN OUTPUT
CRGB leds[NUM_LEDS];

const char ssid[] = "URAWHAT";         // Replace with your SSID
const char passphrase[] = "hacker645";   // Replace with your WPA2 passphrase

ESPAsyncZCPP        zcpp(10);       // ESPAsyncZCPP with X buffers

uint32_t            *seqError;      // Sequence error tracking for each universe
uint32_t            *seqZCPPError;  // Sequence error tracking for each universe
uint16_t            lastZCPPConfig; // last config we saw
uint8_t             seqZCPPTracker; // sequence number of zcpp frames
IPAddress           ourLocalIP;
IPAddress           ourSubnetMask;
WiFiEventHandler    wifiConnectHandler;     // WiFi connect handler
WiFiEventHandler    wifiDisconnectHandler;  // WiFi disconnect handler
IPAddress ip(192, 168, 1, 10);
IPAddress gateway(192, 168, 1, 1); // set gateway to match your network
IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network

const char VERSION[] = "3.1-dev";
const char id[] = "ESPixelStick";


void setup() {
    Serial.begin(115200);
    delay(10);
	// SETUP FASTLED
	FastLED.setBrightness(BRIGHTNESSINDEX);
	FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MILLI_AMPS);
    FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.clear();

	// WIFI SETUP
	WiFi.config(ip, gateway, subnet);
    WiFi.mode(WIFI_STA);
	ConnectWifi();

	// ZCPP INITALIZATION
	ourLocalIP = WiFi.localIP();
	lastZCPPConfig = -1;
	if (zcpp.begin(ourLocalIP)) {
		Serial.println(F("- ZCPP Enabled"));
		ZCPPSub();
	} else {
		Serial.println(F("*** ZCPP INIT FAILED ****"));
	}

}

void loop() {
	ZCPP_packet_t zcppPacket;
	bool doShow = true;

	bool abortPacketRead = false;

	while (!zcpp.isEmpty() && !abortPacketRead) {
		zcpp.pull(&zcppPacket);
		uint8_t seq = zcppPacket.Data.sequenceNumber;
		uint16_t offset = htons(zcppPacket.Data.frameAddress);
		bool frameLast = zcppPacket.Data.flags & ZCPP_DATA_FLAG_LAST;
		uint16_t len = htons(zcppPacket.Data.packetDataLength);
		bool sync = (zcppPacket.Data.flags & ZCPP_DATA_FLAG_SYNC_WILL_BE_SENT) != 0;
		if (sync) {
			// suppress display until we see a sync
			doShow = false;
		}
		if (seq != seqZCPPTracker) {
			Serial.print(F("Sequence Error - expected: "));
			Serial.print(seqZCPPTracker);
			Serial.print(F(" actual: "));
			Serial.println(seq);
			seqZCPPError++;
		}
		if (frameLast)
			seqZCPPTracker = seq + 1;
		zcpp.stats.num_packets++;
		for (int i = 0; i < NUM_LEDS; i++) {
			int j = i * 3;
			leds[i].setRGB(zcppPacket.Data.data[j], zcppPacket.Data.data[j+1], zcppPacket.Data.data[j+2]);
		}
	FastLED.show();
	}
}

void ZCPPSub() {
    ip_addr_t ifaddr;
    ifaddr.addr = static_cast<uint32_t>(WiFi.localIP());
    ip_addr_t multicast_addr;
    multicast_addr.addr = static_cast<uint32_t>(IPAddress(224, 0, 40, 5));
    igmp_joingroup(&ifaddr, &multicast_addr);
}

boolean ConnectWifi(void) {
	boolean state = true;
	int i = 0;
	WiFi.begin(ssid, passphrase);
	Serial.println("");
	Serial.println("Connecting to WiFi");
	Serial.print("Connecting");
	while (WiFi.status() != WL_CONNECTED) {
		leds[i] = CRGB::Blue;
		FastLED.show();
		delay(500);
		Serial.print(".");
		if (i > 10){
			state = false;
			break;
		}
	i++;
	}
	if (state){
		Serial.println("");
		Serial.print("Connected to ");
		Serial.println(ssid);
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
		Connected();
	} else {
		Serial.println("");
		Serial.println("Connection failed.");
		Error();
	}
	FastLED.clear();
	return state;
}

void allOff() {
	fill_solid(leds, NUM_LEDS, CRGB::Black);
	FastLED.show();
	FastLED.clear();
}

void Error() {
	fill_solid(leds, NUM_LEDS, CRGB::Red);
	FastLED.show();
    delay(2000);
    FastLED.clear();
    allOff();
    ConnectWifi();
    //resetFunc(); //call reset
}

void Connected() {
	fill_solid(leds, NUM_LEDS, CRGB::Green);
	FastLED.show();
    delay(1000);
    FastLED.clear();
    allOff();
    FastLED.clear();
}
