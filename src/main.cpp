#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <U8x8lib.h>

#define RST_PIN 0
#define SS_PIN 15
#define BTN_PIN 2

#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 4

U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(16, 5, 4);
U8X8LOG u8x8log;
uint8_t u8logBuffer[U8LOG_WIDTH*U8LOG_HEIGHT];

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

struct uuid { unsigned char bytes[16]; };

const byte BLOCK_UUID = 4;
const byte BLOCK_UUID_TRAILER = 7;

static uuid prevID;
static bool released = false;

char* uuid_to_string(uuid* id, char* out) {
	static const char TOHEXCHAR[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	char* c = out;
	int src_byte = 0;
	for(int i = 0; i < 4; ++i) {
		*c++ = TOHEXCHAR[(id->bytes[src_byte] >> 4) & 0xF];
		*c++ = TOHEXCHAR[id->bytes[src_byte] & 0xF];
		++src_byte;
	}
	*c++ = '-';
	for(int i = 0; i < 2; ++i) {
		*c++ = TOHEXCHAR[(id->bytes[src_byte] >> 4) & 0xF];
		*c++ = TOHEXCHAR[id->bytes[src_byte] & 0xF];
		++src_byte;
	}
	*c++ = '-';
	for(int i = 0; i < 2; ++i) {
		*c++ = TOHEXCHAR[(id->bytes[src_byte] >> 4) & 0xF];
		*c++ = TOHEXCHAR[id->bytes[src_byte] & 0xF];
		++src_byte;
	}
	*c++ = '-';
	for(int i = 0; i < 2; ++i) {
		*c++ = TOHEXCHAR[(id->bytes[src_byte] >> 4) & 0xF];
		*c++ = TOHEXCHAR[id->bytes[src_byte] & 0xF];
		++src_byte;
	}
	*c++ = '-';
	for( int i = 0; i < 6; ++i) {
		*c++ = TOHEXCHAR[(id->bytes[src_byte] >> 4) & 0xF];
		*c++ = TOHEXCHAR[id->bytes[src_byte] & 0xF];
		++src_byte;
	}
    out[36] = '\0';
	return out;
}

bool uuid_from_string(const char* str, uuid* out) {
	char uuid_str[32];
	char* outc = uuid_str;
	for(int i = 0; i < 36; ++i) {
		char c = str[i];
		if(i == 8 || i == 13 || i == 18 || i == 23) {
			if(c != '-' )
				return false;
		}
		else {
			if(!isxdigit(c))
				return false;
			*outc = (char)tolower(c);
			++outc;
		}
	}
	return true;
}

void writeUUID(uuid* id) {
    MFRC522::StatusCode status;
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, BLOCK_UUID_TRAILER, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("error PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(BLOCK_UUID, id->bytes, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("error MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
}

void readUUID(uuid* id) {
    MFRC522::StatusCode status;
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, BLOCK_UUID_TRAILER, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("error PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    byte buf[18];
    byte size = sizeof(buf);
    
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(BLOCK_UUID, buf, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("error MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        readUUID(id);
        return;
    }
    for (int i = 0; i < 16; i++) {
        id->bytes[i] = buf[i];
    }
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

void setup() {
    Serial.begin(115200);
    SPI.begin();
    mfrc522.PCD_Init();
    u8x8.begin();
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8log.begin(u8x8, U8LOG_WIDTH, U8LOG_HEIGHT, u8logBuffer);
  
    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory.
    // Key is needed for authentication when reading/writing blocks.
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    pinMode(BTN_PIN, INPUT_PULLUP);
    Serial.println("ready");
}

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

    Serial.println(id_str);
    for (int i = 0; i < 8; i++) {
        u8x8log.print(id_str[i]);
    }
    u8x8log.print('\n');
}
