#include <Arduino.h>
#include <U8x8lib.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "rfid.h"
#include "game.h"

const static int BTN_PIN = 2;

#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 4

U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(16, 5, 4);
U8X8LOG u8x8log;
uint8_t u8logBuffer[U8LOG_WIDTH*U8LOG_HEIGHT];

static players p;

static bool released = false;

bool buttonPressed() {
    uint64_t t = micros64();
    uint64_t timeout = 1000*20;
    while (micros64() < t + timeout) {
        if (digitalRead(BTN_PIN)) {
            return false;
        }
    }
    return true;
}

void initDisplay() {
    u8x8.begin();
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8log.begin(u8x8, U8LOG_WIDTH, U8LOG_HEIGHT, u8logBuffer);
}

void initWiFi() {
    u8x8log.println("skip wifi?");
    for (int i = 3; i > 0; i--) {
        u8x8log.println(i);
        for (int t = 0; t < 1000; t++) {
            if (buttonPressed()) {
                u8x8log.println("skipping wifi");
                return;
            }
            delay(1);
        }
    }
    u8x8log.println("waiting for wifi");
    WiFiManager manager;
    manager.autoConnect("rfid-ctl");
    u8x8log.println("connected!");
}

void getScoreHTTPS() {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;
    https.begin(*client, "https://nucleate-bee-0940.dataplicity.io/score");
    int httpCode = https.GET();
    String payload = https.getString();
    u8x8log.println(httpCode);
    u8x8log.println(payload);
    Serial.println(httpCode);
    Serial.println(payload);
}

void postStart() {

}

    // StaticJsonBuffer<300> JSONbuffer;
    // JsonObject& JSONencoder = JSONbuffer.createObject();

    // JSONencoder["value"] = value_var;
    // char JSONmessageBuffer[300];
    // JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

    // HTTPClient http;

    // http.begin("https://nucleate-bee-0940.dataplicity.io/score");
    // http.addHeader("Content-Type", "application/json");

    // int httpCode = http.POST(JSONmessageBuffer);
    // String payload = http.getString();

void setup() {
    Serial.begin(115200);
    pinMode(BTN_PIN, INPUT_PULLUP);

    initDisplay();
    u8x8log.println("RFID-CTL");
    initWiFi();
    initRFID();

    getScoreHTTPS();

    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory.
    // Key is needed for authentication when reading/writing blocks.
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println("ready");
    u8x8log.println("ready");

    uuid x = {0x7B,0x0A,0xE2,0x6F,0x5C,0x36,0x49,0xF4,0x96,0xF6,0x79,0x0D,0xA5,0x91,0xDD,0x5A};
    uuid y = {0x53,0x7D,0x6A,0x4F,0xC9,0xB6,0x49,0xF2,0xAA,0xFB,0xE0,0xCD,0xF4,0xBD,0xA1,0x6D};
    clear(&p);
    add(&p, &x);
    add(&p, &y);
    StaticJsonDocument<200> doc;
    json(&p, &doc);
    serializeJson(doc, Serial);
}

void loop() {
    if (digitalRead(BTN_PIN)) {
        released = true;
    }
    if (released && buttonPressed()) {
        released = false;
        Serial.println("start");
        u8x8log.println("start");
    }
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
        return;
    }
    
    uuid id;
    readUUID(&id);
    char id_str[37];
    uuid_to_string(&id, id_str);

    Serial.print("uuid ");
    Serial.println(id_str);
    for (int i = 0; i < 8; i++) {
        u8x8log.print(id_str[i]);
    }
    u8x8log.print('\n');
}
