/*
 * simple_led.ino
 *
 * This accessory contains a builtin-led on NodeMCU and a "virtual" occupancy sensor.
 * GPIO: R 16
 *       G 14
 *       b 12
 * The Flash-Button(D3, GPIO0) on NodeMCU:
 * 		single-click: turn on/off the builtin-led (D4, GPIO2)
 * 		double-click: toggle the occupancy sensor state
 * 		long-click: reset the homekit server (remove the saved pairing)
 *
 *  Created on: 2022-01-20
 *      Author: Ye Zhang
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <arduino_homekit_server.h>
#include "ButtonDebounce.h"
#include "ButtonHandler.h"
#include <Ticker.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <RGBLed.h>

#define R_GPIO 14
#define G_GPIO 12
#define B_GPIO 16
#define key_pin 0
#define led_pin BUILTIN_LED

#define SIMPLE_INFO(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);


ButtonDebounce btn(key_pin, INPUT_PULLUP, LOW);
ButtonHandler btnHandler;
Ticker ticker;

void tick()
{
  //toggle state
  int state = digitalRead(led_pin); // get the current state of GPIO1 pin
  digitalWrite(led_pin, !state);    // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}


void blink_led(int interval, int count) {
	for (int i = 0; i < count; i++) {
		tick();
		delay(interval);
		tick();
		delay(interval);
	}
}

void IRAM_ATTR btnInterrupt() {
	btn.update();
}

void setup()
{
    Serial.begin(115200);

    pinMode(R_GPIO, OUTPUT);
    pinMode(G_GPIO, OUTPUT);
    pinMode(B_GPIO, OUTPUT);
    pinMode(led_pin, OUTPUT);
    digitalWrite(R_GPIO,HIGH);
    digitalWrite(G_GPIO,HIGH);
    digitalWrite(B_GPIO,HIGH);
    btn.setCallback(std::bind(&ButtonHandler::handleChange, &btnHandler,
                              std::placeholders::_1));
    btn.setInterrupt(btnInterrupt);
    btnHandler.setIsDownFunction(std::bind(&ButtonDebounce::checkIsDown, &btn));
    btnHandler.setCallback([](button_event e)
                           {
                               if (e == BUTTON_EVENT_LONGCLICK)
                               {
                                   SIMPLE_INFO("Button Event: LONGCLICK");
                                   SIMPLE_INFO("Rebooting...");
                                   blink_led(300, 3);
                                   homekit_storage_reset();
                                   ESP.restart(); // or system_restart();
                               }
                           });

    ticker.attach(0.6, tick);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset settings - for testing
    //wifiManager.resetSettings();
    //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(configModeCallback);

    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("Homekit_Light"))
    {
        Serial.println("failed to connect and hit timeout");
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    ticker.detach();
    //keep LED off
    digitalWrite(led_pin, HIGH);

	homekit_setup();

}

void loop() {
	homekit_loop();
}


//==============================
// Homekit setup and loop
//==============================

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t name;
extern "C" homekit_characteristic_t serial_number;

#define ChineseWordNumber 5
#define EnglishWordNumber 0
char name_generation[EnglishWordNumber + ChineseWordNumber * 3 + 13] = "灯带控制器";
char categoryNo = 5;
uint8_t MAC_array_STA[6];
char SN[13];
char Setup_ID[5];
char Setup_code[11];


void GenerateSerialNumber(char *_SN)
{
    /*
    *序列号定义
    *SN_   _ _           _               _         _            _          _        _       _
    *    分类编号 (mac[0]+mac[5])%10  mac[1]对应字母 mac[2]%10 mac[3]%10  mac[4]%10  随机数  随机字母
    */ 
    char words[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";  //36个字符

    
    _SN[0] = 'S';
    _SN[1] = 'N';
    _SN[2] = '_';
    _SN[3] = (char)(categoryNo/10) % 10 + 48;
    _SN[4] = categoryNo%10 + 48;
    _SN[5] = (MAC_array_STA[0]+MAC_array_STA[5])%10 + 48;
    _SN[6] = words[MAC_array_STA[1]%36];
    _SN[7] = MAC_array_STA[2]%10 + 48;
    _SN[8] = words[(MAC_array_STA[3]*MAC_array_STA[2] +6)%36];
    _SN[9] = MAC_array_STA[4]%10 + 48;
    _SN[10] = (char)((MAC_array_STA[3] * MAC_array_STA[2] + MAC_array_STA[2] * 0.86425) * 0.5)%10 + 48;
    _SN[11] = words[((char)((MAC_array_STA[5] * 6 / MAC_array_STA[2] + MAC_array_STA[4] * 0.86425) * 0.5 +0.5))%36];
    _SN[12] = '\0';
}

void GenerateSetupPassword(char *_setupID, char *_setupcode)
{
    char words[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";  //36个字符

    _setupID[0] = words[((char)((MAC_array_STA[5] * 4.3 / MAC_array_STA[3] + MAC_array_STA[0] * 0.86425) * 0.5 +0.5)) % 36];
    _setupID[1] = words[((char)((MAC_array_STA[1] * 2 / (MAC_array_STA[2] * 0.3) + MAC_array_STA[4] * 3) * 0.9 +0.5)) % 36];
    _setupID[2] = words[((char)((MAC_array_STA[3] * MAC_array_STA[2] +425) * 0.5 +0.5)) % 36];
    _setupID[3] = words[((char)((MAC_array_STA[4] / MAC_array_STA[1] + MAC_array_STA[5] * 0.86425) * 0.5 +25.5))%36];
    _setupID[4] = '\0';
    /* Unique Setup code of the format xxx-xx-xxx. Default: 111-22-333 */
    _setupcode[0] = (MAC_array_STA[0]+MAC_array_STA[5])%10 + 48;
    _setupcode[1] = MAC_array_STA[2]%10 + 48;
    _setupcode[2] =  ((char)((MAC_array_STA[3] * MAC_array_STA[2] + MAC_array_STA[2] * 0.86425) * 0.5)%10) + 48;
    _setupcode[3] = 0x2d;
    _setupcode[4] = (categoryNo * MAC_array_STA[0] + MAC_array_STA[5])%10 + 48;
    _setupcode[5] = (categoryNo * 16 + MAC_array_STA[5] * MAC_array_STA[1])%10 + 48;
    _setupcode[6] = 0x2d;
    _setupcode[7] = (MAC_array_STA[0] + MAC_array_STA[1] + MAC_array_STA[2] + MAC_array_STA[3] + MAC_array_STA[4] + MAC_array_STA[5]) % 10 + 48;
    _setupcode[8] = ((MAC_array_STA[0] + MAC_array_STA[1] + MAC_array_STA[2]) * (MAC_array_STA[3] + MAC_array_STA[4] + MAC_array_STA[5])) % 10 + 48;
    _setupcode[9] = ((MAC_array_STA[0] + MAC_array_STA[5] + MAC_array_STA[3]) * (MAC_array_STA[3] + MAC_array_STA[2] + MAC_array_STA[5])) % 10 + 48;
    _setupcode[10] = '\0';
}



void homekit_setup() {

	WiFi.macAddress(MAC_array_STA);
    GenerateSerialNumber(SN);
    for (int i = EnglishWordNumber + ChineseWordNumber * 3; i < EnglishWordNumber + ChineseWordNumber * 3 + 13; i++)
    {
        if (i-(EnglishWordNumber + ChineseWordNumber * 3) < 13)
        {
            name_generation[i] = SN[i-(EnglishWordNumber + ChineseWordNumber * 3)];
        }
    }
	name.value = HOMEKIT_STRING_CPP(name_generation);
    serial_number.value =  HOMEKIT_STRING_CPP(SN);
    GenerateSetupPassword(Setup_ID, Setup_code);
    Serial.println(name_generation);
    Serial.print("Setup_code:");
    Serial.print(Setup_code);
    Serial.print("  Setup_ID:");
    Serial.println(Setup_ID);
    config.password = Setup_code;
    config.setupId = Setup_ID;

	arduino_homekit_setup(&config);
}

void homekit_loop() {
	btnHandler.loop();
	arduino_homekit_loop();
	static uint32_t next_heap_millis = 0;
	uint32_t time = millis();
	if (time > next_heap_millis) {
		SIMPLE_INFO("heap: %d, sockets: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
		next_heap_millis = time + 5000;
	}
}
