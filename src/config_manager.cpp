#include "config_manager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// Definizione dell'istanza globale
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

    // Carica i valori e applica TRIM per rimuovere spazi
    String wkey = String(doc["weatherKey"] | "");
    String city = String(doc["weatherCity"] | "Rome");
    String country = String(doc["weatherCountry"] | "IT");
    String tz = String(doc["timezone"] | "CET-1CEST,M3.5.0,M10.5.0/3");
    
    wkey.trim();
    city.trim();
    country.trim();
    tz.trim();
    
    strlcpy(data.weatherKey, wkey.c_str(), sizeof(data.weatherKey));
    strlcpy(data.weatherCity, city.c_str(), sizeof(data.weatherCity));
    strlcpy(data.weatherCountry, country.c_str(), sizeof(data.weatherCountry));
    strlcpy(data.timezone, tz.c_str(), sizeof(data.timezone));

    file.close();
    Serial.printf("Config caricata: City='%s' Country='%s'\n", data.weatherCity, data.weatherCountry);
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