#include <Arduino.h>
#include <TFT_eSPI.h>

// Networking imports
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h> 
#include <WebServer.h>

#include "Free_Fonts.h"
#include "image.h"

#include "rfid.h"
#include "game.h"

#define RGB565(r,g,b) ((((r>>3)<<11) | ((g>>2)<<5) | (b>>3)))

#define HeaderBackground RGB565(51, 143, 255)
#define Primary RGB565(0, 134, 191)
#define Highlight RGB565(128, 185, 255)
#define Highlight2 RGB565(199, 255, 154)

#define Black RGB565(0, 0, 0)
#define Red	RGB565(255, 0, 0)
#define Green RGB565(0, 255, 0)
#define Blue RGB565(0, 0, 255)
#define Gray RGB565(128, 128, 128)
#define LighterRed RGB565(255, 150, 150)
#define LighterGreen RGB565(150, 255, 150)
#define LighterBlue RGB565(150, 150, 255)
#define DarkerRed RGB565(150, 0, 0)
#define DarkerGreen RGB565(0, 150, 0)
#define DarkerBlue RGB565(0, 0, 150)
#define Cyan RGB565(0, 255, 255)
#define Magenta RGB565(255, 0, 255)
#define Yellow RGB565(255, 255, 0)
#define White RGB565(255, 255, 255)

const static int pwmFreq = 5000;
const static int pwmResolution = 8;
const static int pwmLedChannelTFT = 0;

const static int STATE_BOOT = 0;
const static int STATE_CONNECT = 1;
const static int STATE_SCAN = 2;
const static int STATE_START = 3;
const static int STATE_ERROR = 4;
const static int STATE_SUCCESS = 5;
const static int STATE_PROGRAM = 6;

const static int BTN_PIN = 17;

struct input {
    bool click;
    bool increment;
    bool decrement;
};

// AsyncWebServer server(80);

static players p;

static char err[256];
static char buf[256];
static int bufIndex = 0;

static bool released = false;

int ledBacklight = 255;

TFT_eSPI tft = TFT_eSPI();
WiFiClientSecure client;

const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"\
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"\
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"\
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"\
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"\
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"\
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"\
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"\
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"\
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"\
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"\
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"\
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"\
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"\
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"\
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"\
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"\
"rqXRfboQnoZsG4q5WTP468SQvvG5\n"\
"-----END CERTIFICATE-----\n";               

const char* hostname = "nucleate-bee-0940.dataplicity.io";

bool buttonPressed() {
    uint64_t t = micros();
    uint64_t timeout = 1000*5;
    while (micros() < t + timeout) {
        if (digitalRead(BTN_PIN)) {
            return false;
        }
    }
    return true;
}

void bootscreen() {
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 135, bootlogo);
}

void errorscreen() {
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 135, errorimg);
    tft.setFreeFont(FSSB9);
    tft.setCursor(10, 20);
    tft.print("Error: ");
    tft.print(err);
}

void successscreen() {
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 135, connectedimg);
}

void connectingscreen() {
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 135, connectingimg);
}

void scanscreen() {
    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 135, scanimg);
    tft.setFreeFont(FSSB9);
    tft.setCursor(0, 0);
    tft.print("\n\n");
    for (int n = 0; n < p.count; n++) {
        char id_str[37];
        uuid_to_string(&(p.ids[n]), id_str);
        tft.print("    ");
        for (int i = 0; i < 8; i++) {
            tft.print(id_str[i]);
        }
        tft.print('\n');
    }
    tft.setCursor(65, 130);
    tft.println("Click to start");
}

void startscreen() {
    tft.fillScreen(White);
    tft.setFreeFont(FSSB12);
    tft.setCursor(0, 30);
    tft.println("DEBUG INFO");
    tft.setFreeFont(FSSB9);
    tft.println("Connecting...");
}

void programscreen() {
    tft.fillScreen(White);
    tft.setFreeFont(FSSB12);
    tft.setCursor(0, 30);
    tft.println("WRITE MODE");
    tft.setFreeFont(FSSB9);
    tft.println("Tap to program card.");
    tft.print("UUID: ");
    for (int i = 0; i < 8; i++) {
        tft.print(buf[i]);
    }
}

void programmedscreen() {
    tft.fillScreen(White);
    tft.setFreeFont(FSSB12);
    tft.setCursor(0, 30);
    tft.println("Card programmed");
}

void initDisplay() {
    tft.init();
    tft.fillScreen(White);
    tft.setRotation(3);
    tft.setTextSize(1);
    tft.setTextWrap(false);
    tft.setFreeFont(FSSB9);
    tft.setTextColor(Black, White);
    ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
    ledcAttachPin(TFT_BL, pwmLedChannelTFT);
    ledcWrite(pwmLedChannelTFT, ledBacklight);
}

int scan() {
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return 1;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
        return 1;
    }

    uuid id;
    if (readUUID(&id)) {
        return 1;
    };
    char id_str[37];
    uuid_to_string(&id, id_str);

    if (!contains(&p, &id) && p.count < 4) {
        add(&p, &id);
        Serial.print("uuid ");
        Serial.println(id_str);
        return 0;
    }
    return 1;
}

int program() {
    if (!mfrc522.PICC_IsNewCardPresent()) {
        return 1;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
        return 1;
    }

    uuid id;
    uuid_from_string(buf, &id);

    int err = writeUUID(&id);

    char str[50];
    uuid_to_string(&id, str);

    Serial.println(str);
    return err;
}

void initWiFi() {
    WiFiManager wifiManager;
    wifiManager.autoConnect("parsable-rfid-config");
    Serial.println("connected!");
    client.setCACert(root_ca);
    Serial.println(WiFi.localIP());
}

bool pongboardStart() {
    Serial.println("\nStarting connection to server...");

    if (!client.connect(hostname, 443)) {
        Serial.println("Connection failed!");
        tft.println("Connection failed!");
        delay(2000);
        return 1;
    }
    else {
        StaticJsonDocument<512> doc;
        doc.clear();
        JsonObject root = doc.to<JsonObject>();
        JsonArray ids = root.createNestedArray("players");

        for (int n = 0; n < p.count; n++) {
            char id_str[37];
            uuid_to_string(&(p.ids[n]), id_str);
            ids.add(id_str);
        }
        serializeJsonPretty(root, Serial);
        Serial.println("");
        String payload;
        serializeJson(doc, payload);

        tft.println("Sending payload...");
        Serial.println("Connected to server!");
        client.println("POST https://nucleate-bee-0940.dataplicity.io/start2 HTTP/1.0");
        client.println("Content-Type: application/json");
        client.println("Accept: application/json");
        client.println("Host: nucleate-bee-0940.dataplicity.io");
        client.println("Connection: close");
        client.println("Content-Length: " + String(payload.length()));
        client.println();
        client.println(payload);
        client.println();

        while (client.connected()) {
            String line = client.readStringUntil('\n');
            if (line == "\r") {
                Serial.println("headers received");
                tft.println("Got response from server.");
                break;
            }
        }
  
        while (client.available()) {
            char c = client.read();
            Serial.write(c);
            tft.print(c);
        }
        client.stop();
        delay(3000);
        return 0;
    }
}

bool pongboardReset() {
    Serial.println("\nStarting connection to server...");
    if (!client.connect(hostname, 443)) {
        Serial.println("Connection failed!");
        return 1;
    }
    else {
        Serial.println("Connected to server!");
        client.println("POST https://nucleate-bee-0940.dataplicity.io/reset HTTP/1.0");
        client.println("Host: nucleate-bee-0940.dataplicity.io");
        client.println("Connection: close");
        client.println();

        while (client.connected()) {
            String line = client.readStringUntil('\n');
            if (line == "\r") {
                tft.println("Got response from server.");
                Serial.println("headers received");
                break;
            }
        }
        while (client.available()) {
            char c = client.read();
            Serial.write(c);
        }
        client.stop();
        return 0;
    }
}

int next(int state, input in) {
    switch (state) {
    case STATE_BOOT:
        bootscreen();
        delay(4000);
        scanscreen();
        return STATE_CONNECT;
    case STATE_CONNECT:
        connectingscreen();
        delay(1000);
        initWiFi();
        scanscreen();
        return STATE_SCAN;
    case STATE_SCAN:
        if (in.click) {
            startscreen();
            return STATE_START;
        }
        if (!scan()) {
            scanscreen();
        }
        return STATE_SCAN;
    case STATE_START:
        if (p.count != 2 && p.count != 4) {
            p.count = 0;
            sprintf(err, "scan 2 or 4 cards");
            return STATE_ERROR;    
        }
        if (!pongboardStart()) {
            p.count = 0;
            return STATE_SUCCESS;
        }
        p.count = 0;
        sprintf(err, "unknown error occurred");
        return STATE_ERROR;
    case STATE_ERROR:
        errorscreen();
        delay(3000);
        scanscreen();
        return STATE_SCAN;
    case STATE_SUCCESS:
        successscreen();
        delay(3000);
        scanscreen();
        return STATE_SCAN;
    case STATE_PROGRAM:
        if (!program()) {
            Serial.println("Programmed card");
            programmedscreen();
            delay(2000);
            p.count = 0;
            scanscreen();
            return STATE_SCAN;
        }
        return STATE_PROGRAM;
    default:
        return state;
    }
}
// 00112233-4455-6677-8899-aabbccddeeff
// 00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff

void uuidTest() {
    //  abcdefgh-1234-9876-0000-000111000111
    char buf[50] = "8a8a4a44-21b7-4992-aae7-c1f56e97261e";
    char out[50];

    uuid id;
    uuid_from_string(buf, &id);

    uuid_to_string(&id, out);
    Serial.println("ORIGINAL UUID");
    Serial.println(buf);
    Serial.println("CONVERTED UUID");
    Serial.println(out);


}


void setup() {
    Serial.begin(115200);
    while (Serial.available()) {
        Serial.read();
    }
    initDisplay();
    initRFID();
    uuidTest();
    

    pinMode(BTN_PIN, INPUT_PULLUP);
	
    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory.
    // Key is needed for authentication when reading/writing blocks.
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println("ready");
}


void loop() {
    static int state = STATE_BOOT;
    input in;

    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            buf[bufIndex] = '\0';
            if (bufIndex == 37) {
                Serial.println("Got a UUID");
                Serial.println(buf);
                programscreen();
                state = STATE_PROGRAM;
            } else {
                Serial.print("Unrecognized string: ");
                Serial.println(buf);
                Serial.println(bufIndex);
            }
            bufIndex = 0;
        } else {
            buf[bufIndex] = c;
            bufIndex++;
        }
    }

    if (digitalRead(BTN_PIN)) {
        released = true;
    }
    if (released && buttonPressed()) {
        released = false;
        in.click = true;
        Serial.println("start");
    } else {
        in.click = false;
    }
    int old = state;
    state = next(state, in);
    if (state != old) {
        Serial.print("state transition: old ");
        Serial.print(old);
        Serial.print(" new ");
        Serial.println(state);
    }

}
