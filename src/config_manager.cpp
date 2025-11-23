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

bool ConfigManager::loadConfig() {
    if (!LittleFS.exists(filename)) {
        Serial.println("Config non trovata, uso default.");
        saveConfig();
        return true;
    }

    File file = LittleFS.open(filename, "r");
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);

    if (error) {
        Serial.println("Errore lettura JSON");
        return false;
    }

    strlcpy(data.weatherKey, doc["weatherKey"] | "", sizeof(data.weatherKey));
    strlcpy(data.weatherCity, doc["weatherCity"] | "Rome", sizeof(data.weatherCity));
    strlcpy(data.weatherCountry, doc["weatherCountry"] | "IT", sizeof(data.weatherCountry));
    strlcpy(data.timezone, doc["timezone"] | "CET-1CEST,M3.5.0,M10.5.0/3", sizeof(data.timezone));

    file.close();
    Serial.println("Config caricata.");
    return true;
}

bool ConfigManager::saveConfig() {
    StaticJsonDocument<512> doc;
    doc["weatherKey"] = data.weatherKey;
    doc["weatherCity"] = data.weatherCity;
    doc["weatherCountry"] = data.weatherCountry;
    doc["timezone"] = data.timezone;

    File file = LittleFS.open(filename, "w");
    if (!file) return false;

    serializeJson(doc, file);
    file.close();
    Serial.println("Config salvata.");
    return true;
}

void ConfigManager::resetToDefault() {
    AppConfig defaultConfig;
    data = defaultConfig;
    saveConfig();
}