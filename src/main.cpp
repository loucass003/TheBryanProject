#include <Arduino.h>
#include "DFRobotDFPlayerMini.h"
#include <WiFi.h>
#include <AutoConnect.h>
#include "ArduinoOTA.h"
#include <ESP32Servo.h>
#include <FastLED.h>
#include "Button2.h"


#define NUM_LEDS 6
#define DATA_PIN 5
#define BUTTON_PIN 22
#define BUTTON_LED_PIN 18
#define SERVO_PIN 23

HardwareSerial mySoftwareSerial(1);
DFRobotDFPlayerMini myDFPlayer;
Servo armServo;
WebServer portalServer;
AutoConnect portal(portalServer);
AutoConnectConfig apConfig("THEBRYANPROJECT", "12345678");

int songsCount = 6;

// Define the array of leds
CRGBArray<NUM_LEDS> leds;

Button2 button;

int playing = 0;
int done = 1;
uint16_t animationSpeedDelay = 0;

void buttonPressed(Button2& btn) {
	if (playing == 0) {
		done = 0;
		animationSpeedDelay = max(100, random16(8) * 100);
		myDFPlayer.play((millis() % songsCount) + 1);
		playing = 1;
	} else {
		done = 1;
		playing = 0;
		myDFPlayer.stop();
	}
}


void initOTA() {
	ArduinoOTA.setHostname("THEBRYANPROJECT");
	ArduinoOTA.onStart([]() {
		Serial.println("Start");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nEnd");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();
}


void onConnect(IPAddress& clientIP) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    initOTA();
}

void setup()
{
	Serial.begin(115200);

	button.begin(BUTTON_PIN);
  	button.setTapHandler(buttonPressed);
	apConfig.ota = AC_OTA_EXTRA;
	apConfig.portalTimeout = 1;
	apConfig.retainPortal = true;
	portal.config(apConfig);
	portal.onConnect(onConnect);
	portal.whileCaptivePortal([] () { return true; });
	portal.begin();

	armServo.attach(SERVO_PIN);
	FastLED.addLeds<WS2812, DATA_PIN, BRG>(leds, NUM_LEDS);
	FastLED.setBrightness(255);
	mySoftwareSerial.begin(9600, SERIAL_8N1, 16, 17);  // speed, type, RX, TX

	
	Serial.println();
	Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
	
	while(!myDFPlayer.begin(mySoftwareSerial)) {
		Serial.println(F("Unable to begin"));
	}
	Serial.println(F("DFPlayer Mini online."));

	songsCount = myDFPlayer.readFileCounts();
	myDFPlayer.volume(30);
	myDFPlayer.disableLoop();
	myDFPlayer.EQ(DFPLAYER_EQ_BASS);
	// myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);


	pinMode(BUTTON_LED_PIN, OUTPUT);

	pinMode(BUTTON_PIN, INPUT_PULLUP);

	randomSeed(analogRead(0));

	// myDFPlayer.play(1);

	
}


int pos = 0;

int lastToggle = 0;
int toggle = 0;
int animationFrame = 0;
int animationDelay = 0;
int press = 0;
int buttonBlink = 0;

uint8_t hue = 0;

void loop()
{
	portal.handleClient();
	if (WiFi.status() == WL_CONNECTED) {
    	ArduinoOTA.handle();
	}
	button.loop();

	if (millis() - animationDelay > 60) {
		if (playing) {
			if (buttonBlink) {
				for(int i = 0; i < NUM_LEDS/2; i++) {   
					leds.fadeToBlackBy(40);
					leds[i] = CHSV(hue++,255,255);
					leds(NUM_LEDS/2,NUM_LEDS-1) = leds(NUM_LEDS/2 - 1 ,0);
				}
			}
			else {
				FastLED.clear();
			}
		
		
			buttonBlink = !buttonBlink;
		} else {
			for(int i = 0; i < NUM_LEDS/2; i++) {   
				leds.fadeToBlackBy(40);
				leds[i] = CHSV(hue++,255,255);
				leds(NUM_LEDS/2,NUM_LEDS-1) = leds(NUM_LEDS/2 - 1 ,0);
			}
			buttonBlink = 0;
		}
		animationDelay = millis();
	}
	
	
	int start = 100;
	int offset = 30;

	if (playing && animationSpeedDelay && millis() - lastToggle > animationSpeedDelay) {
		toggle = toggle == 0 ? 1 : 0;

		if (toggle == 0) {
			armServo.write(start + offset); 
		} else {
			armServo.write(start - offset);
		}

		lastToggle = millis();
	} else if (!playing) {
		armServo.write(start);
	}

	if (myDFPlayer.available()) {
		if (done != 1 && myDFPlayer.readType() == DFPlayerPlayFinished) {
			done = 1;
			playing = 0;
		}
	}


	digitalWrite(BUTTON_LED_PIN, buttonBlink ? HIGH : LOW);
	FastLED.show();
}

