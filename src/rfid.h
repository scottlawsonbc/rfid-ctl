#ifndef UUID_H
#define UUID_H

#include <SPI.h>
#include <MFRC522.h>

constexpr static int SS_PIN = 15;
constexpr static int RST_PIN = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

struct uuid { unsigned char bytes[16]; };

const byte BLOCK_UUID = 4;
const byte BLOCK_UUID_TRAILER = 7;

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

void initRFID() {
    SPI.begin();
    mfrc522.PCD_Init();
}


#endif