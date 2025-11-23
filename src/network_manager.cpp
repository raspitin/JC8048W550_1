#include "network_manager.h"
#include "config_manager.h"
#include <WiFiManager.h>

bool shouldSaveConfig = false;

void saveConfigCallback() {
    Serial.println("Configurazione modificata nel portale -> Salvataggio richiesto.");
    shouldSaveConfig = true;
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
        strlcpy(configManager.data.weatherKey, custom_wkey.getValue(), sizeof(configManager.data.weatherKey));
        strlcpy(configManager.data.weatherCity, custom_city.getValue(), sizeof(configManager.data.weatherCity));
        strlcpy(configManager.data.weatherCountry, custom_country.getValue(), sizeof(configManager.data.weatherCountry));
        strlcpy(configManager.data.timezone, custom_tz.getValue(), sizeof(configManager.data.timezone));
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