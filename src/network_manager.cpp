#include "network_manager.h"
#include "config_manager.h"
#include <WiFiManager.h>

bool shouldSaveConfig = false;

void saveConfigCallback() {
    Serial.println("Configurazione modificata nel portale -> Salvataggio richiesto.");
    shouldSaveConfig = true;
}

// Funzione helper per rimuovere spazi iniziali e finali
String trimString(const char* str) {
    String s = String(str);
    s.trim();
    return s;
}

bool setup_network() {
    // Carica le impostazioni precedenti
    configManager.begin(); 

    WiFiManager wm;

    // Parametri Custom
    WiFiManagerParameter custom_wkey("wkey", "OpenWeather API Key", configManager.data.weatherKey, 64);
    WiFiManagerParameter custom_city("city", "Citta (es. Rome)", configManager.data.weatherCity, 32);
    WiFiManagerParameter custom_country("country", "Paese (es. IT)", configManager.data.weatherCountry, 4);
    WiFiManagerParameter custom_tz("tz", "Timezone", configManager.data.timezone, 64);

    wm.addParameter(&custom_wkey);
    wm.addParameter(&custom_city);
    wm.addParameter(&custom_country);
    wm.addParameter(&custom_tz);

    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setDebugOutput(true);
    wm.setConfigPortalTimeout(180); 

    // Avvio connessione automatica
    bool res = wm.autoConnect("Termostato_Setup");

    if (!res) {
        Serial.println("Connessione fallita o timeout scaduto.");
        return false;
    }

    Serial.println("WiFi Connesso!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    if (shouldSaveConfig) {
        // TRIM dei valori per rimuovere spazi indesiderati
        String wkey = trimString(custom_wkey.getValue());
        String city = trimString(custom_city.getValue());
        String country = trimString(custom_country.getValue());
        String tz = trimString(custom_tz.getValue());
        
        strlcpy(configManager.data.weatherKey, wkey.c_str(), sizeof(configManager.data.weatherKey));
        strlcpy(configManager.data.weatherCity, city.c_str(), sizeof(configManager.data.weatherCity));
        strlcpy(configManager.data.weatherCountry, country.c_str(), sizeof(configManager.data.weatherCountry));
        strlcpy(configManager.data.timezone, tz.c_str(), sizeof(configManager.data.timezone));
        
        Serial.printf("Salvati: City='%s' Country='%s'\n", city.c_str(), country.c_str());
        
        configManager.saveConfig();
    }

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", configManager.data.timezone, 1);
    tzset();

    return true;
}

void wifi_reset_settings() {
    WiFiManager wm;
    wm.resetSettings();
    Serial.println("Impostazioni WiFi cancellate.");
    delay(1000);
    ESP.restart();
}