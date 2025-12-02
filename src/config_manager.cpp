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
    strlcpy(data.weatherKey, "3737538a12e713a7bd87f314c9086bc2", sizeof(data.weatherKey));
    strlcpy(data.weatherCity, "Rome", sizeof(data.weatherCity));
    strlcpy(data.weatherCountry, "IT", sizeof(data.weatherCountry));
    strlcpy(data.timezone, "CET-1CEST,M3.5.0,M10.5.0/3", sizeof(data.timezone));

    // Default: Acceso dalle 06:00 (slot 12) alle 08:30 (slot 17) 
    // e dalle 17:30 (slot 35) alle 22:30 (slot 45)
    // 06:00 è 6*2 = 12. 08:30 è 8*2+1 = 17.
    uint64_t defaultSchedule = 0;
    for(int i=12; i<17; i++) defaultSchedule |= (1ULL << i);
    for(int i=35; i<45; i++) defaultSchedule |= (1ULL << i);

    for(int i=0; i<7; i++) {
        data.weekSchedule[i].timeSlots = defaultSchedule;
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

    JsonArray week = doc["schedule"];
    if(!week.isNull()) {
        for(int d=0; d<7; d++) {
            // Carichiamo High e Low part per ricostruire uint64
            uint32_t h = week[d]["h"] | 0;
            uint32_t l = week[d]["l"] | 0;
            data.weekSchedule[d].timeSlots = ((uint64_t)h << 32) | l;
        }
    } else {
        resetToDefault(); 
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

    JsonArray week = doc.createNestedArray("schedule");
    for(int d=0; d<7; d++) {
        JsonObject dayObj = week.createNestedObject();
        // Salviamo come due interi a 32 bit per evitare problemi con JS/JSON
        dayObj["h"] = (uint32_t)(data.weekSchedule[d].timeSlots >> 32);
        dayObj["l"] = (uint32_t)(data.weekSchedule[d].timeSlots & 0xFFFFFFFF);
    }

    File file = LittleFS.open(filename, "w");
    if (!file) return false;

    serializeJson(doc, file);
    file.close();
    Serial.println("Config salvata.");
    return true;
}