#include "config_manager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

ConfigManager configManager;
const char* filename = "/config.json";

bool ConfigManager::begin() {
    if (!LittleFS.begin(true)) { 
        Serial.println("LittleFS Mount Failed");
        return false;
    }
    return loadConfig();
}

void ConfigManager::resetToDefault() {
    // Default: Weather
    strlcpy(data.weatherKey, "3737538a12e713a7bd87f314c9086bc2", sizeof(data.weatherKey));
    strlcpy(data.weatherCity, "Rome", sizeof(data.weatherCity));
    strlcpy(data.weatherCountry, "IT", sizeof(data.weatherCountry));
    strlcpy(data.timezone, "CET-1CEST,M3.5.0,M10.5.0/3", sizeof(data.timezone));

    // Default: Programma standard per tutti i giorni
    // 06:00 -> 21.0°C
    // 08:30 -> 18.0°C
    // 17:30 -> 21.5°C
    // 22:30 -> 17.0°C
    for(int i=0; i<7; i++) {
        data.weekSchedule[i].slots[0] = {6,  0,  21.0};
        data.weekSchedule[i].slots[1] = {8,  30, 18.0};
        data.weekSchedule[i].slots[2] = {17, 30, 21.5};
        data.weekSchedule[i].slots[3] = {22, 30, 17.0};
    }
    saveConfig();
}

bool ConfigManager::loadConfig() {
    if (!LittleFS.exists(filename)) {
        Serial.println("Config non trovata, reset default.");
        resetToDefault();
        return true;
    }

    File file = LittleFS.open(filename, "r");
    // Aumentiamo buffer per contenere il programma settimanale
    DynamicJsonDocument doc(4096); 
    DeserializationError error = deserializeJson(doc, file);

    if (error) {
        Serial.println("Errore JSON, resetto.");
        resetToDefault();
        return false;
    }

    strlcpy(data.weatherKey, doc["w_key"] | "3737538a12e713a7bd87f314c9086bc2", sizeof(data.weatherKey));
    strlcpy(data.weatherCity, doc["w_city"] | "Rome", sizeof(data.weatherCity));
    strlcpy(data.weatherCountry, doc["w_country"] | "IT", sizeof(data.weatherCountry));
    strlcpy(data.timezone, doc["tz"] | "CET-1CEST,M3.5.0,M10.5.0/3", sizeof(data.timezone));

    // Carica Programma
    JsonArray week = doc["schedule"];
    if(!week.isNull()) {
        for(int d=0; d<7; d++) {
            JsonArray daySlots = week[d];
            if(!daySlots.isNull()) {
                for(int s=0; s<4; s++) {
                    data.weekSchedule[d].slots[s].hour   = daySlots[s]["h"] | 0;
                    data.weekSchedule[d].slots[s].minute = daySlots[s]["m"] | 0;
                    data.weekSchedule[d].slots[s].temp   = daySlots[s]["t"] | 18.0;
                }
            }
        }
    } else {
        // Se manca la schedule nel JSON vecchio, resetta solo quella
        resetToDefault(); // O gestisci più finemente
    }

    file.close();
    Serial.println("Config caricata.");
    return true;
}

bool ConfigManager::saveConfig() {
    DynamicJsonDocument doc(4096);
    doc["w_key"] = data.weatherKey;
    doc["w_city"] = data.weatherCity;
    doc["w_country"] = data.weatherCountry;
    doc["tz"] = data.timezone;

    // Salva Programma (Array di Array)
    JsonArray week = doc.createNestedArray("schedule");
    for(int d=0; d<7; d++) {
        JsonArray daySlots = week.createNestedArray();
        for(int s=0; s<4; s++) {
            JsonObject slot = daySlots.createNestedObject();
            slot["h"] = data.weekSchedule[d].slots[s].hour;
            slot["m"] = data.weekSchedule[d].slots[s].minute;
            slot["t"] = data.weekSchedule[d].slots[s].temp;
        }
    }

    File file = LittleFS.open(filename, "w");
    if (!file) return false;

    serializeJson(doc, file);
    file.close();
    Serial.println("Config salvata.");
    return true;
}