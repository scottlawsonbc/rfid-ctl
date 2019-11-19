#ifndef UUID_H
#define UUID_H

#include <SPI.h>
#include <MFRC522.h>

constexpr static int VSPI_SCK = 26;
constexpr static int VSPI_MISO = 33;
constexpr static int VSPI_MOSI = 25;
constexpr static int VSPI_SS = 27;

constexpr static int RST_PIN = 32;

MFRC522 mfrc522(VSPI_SS, RST_PIN);
MFRC522::MIFARE_Key key;

// SL uuid 8a8a4a44-21b7-4992-aae7-c1f56e97261e
// BK uuid 4dfc22fb-1f22-43db-bdd0-001e7f4e49fe
// JU uuid c833cc7b-48f2-4279-a18d-62bfa7c0dc6e

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

int readUUID(uuid* id) {
    MFRC522::StatusCode status;
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, BLOCK_UUID_TRAILER, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("error PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return 1;
    }
    byte buf[18];
    byte size = sizeof(buf);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(BLOCK_UUID, buf, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("error MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        readUUID(id);
        return 1;
    }
    for (int i = 0; i < 16; i++) {
        id->bytes[i] = buf[i];
    }
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
	return 0;
}

void initRFID() {
    SPI.begin();
	MFRC522_SPI.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, VSPI_SS);
	mfrc522.PCD_Init();
}


#endif