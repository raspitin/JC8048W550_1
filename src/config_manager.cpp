#include "config_manager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// Istanza globale definita qui (dichiarata extern nell'header)
ConfigManager configManager;

const char* filename = "/config.json";

bool ConfigManager::begin() {
    if (!LittleFS.begin(true)) { // true = formatta se fallisce il mount
        Serial.println("LittleFS Mount Failed");
        return false;
    }
    return loadConfig();
}

bool ConfigManager::loadConfig() {
    if (!LittleFS.exists(filename)) {
        Serial.println("Config file non trovato, creo default.");
        saveConfig(); // Crea il file di default se manca
        return true;
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.println("Impossibile aprire config file");
        return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);

    if (error) {
        Serial.println("Errore lettura JSON");
        file.close();
        return false;
    }

    // Carica i dati dal JSON alla struct, usando i valori attuali come default se mancano campi
    strlcpy(data.weatherKey, doc["weatherKey"] | "3737538a12e713a7bd87f314c9086bc2", sizeof(data.weatherKey));
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
    if (!file) {
        Serial.println("Fallita apertura file per scrittura");
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Fallita scrittura JSON");
        file.close();
        return false;
    }
    
    file.close();
    Serial.println("Config salvata.");
    return true;
}

void ConfigManager::resetToDefault() {
    AppConfig defaultConfig; // Crea una struct con i valori di default
    data = defaultConfig;    // Sovrascrive i dati attuali
    saveConfig();            // Salva su disco
}