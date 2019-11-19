#ifndef GAME_H
#define GAME_H

#include <ArduinoJson.h>
#include "rfid.h"

constexpr static int MAX_PLAYERS = 4;
char uuid_str[MAX_PLAYERS][37];

struct players { 
    uuid ids[MAX_PLAYERS];
    int count;
};

bool contains(players* p, uuid* id) {
    for (int n = 0; n < p->count; n++) {
        if (!memcmp(p->ids[n].bytes, id->bytes, 16)) {
            return true;
        }
        // for (int i = 0; i < 16; i++) {
        //     Serial.print(i);
        //     Serial.print(" ");
        //     Serial.print(p->ids[n].bytes[i]);
        //     Serial.print(" ");
        //     Serial.print(id->bytes[i]);
        //     Serial.print("\n");
   
        // }

    }
    return false;
}

void add(players* p, uuid* id) {
    if (p->count >= MAX_PLAYERS) {
        return;
    }
    memcpy(p->ids[p->count].bytes, id->bytes, 16);
    p->count++;
}

void clear(players* p) {
    p->count = 0;
}

void json(players* p, JsonDocument* doc) {
    doc->clear();
    JsonArray ids = doc->createNestedArray("ids");

    for (int n = 0; n < p->count; n++) {
        uuid_to_string(&(p->ids[n]), uuid_str[n]);
        ids.add(uuid_str[n]);
    }
}

#endif