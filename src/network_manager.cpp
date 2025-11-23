#include "network_manager.h"
#include "config_manager.h"
#include "ui.h"
#include <WiFiManager.h>

WiFiManager wm;
bool shouldSaveConfig = false;

void saveConfigCallback() {
    shouldSaveConfig = true;
}

bool setup_network() {
    configManager.begin(); // Carica config esistente

    // Parametri custom per la pagina di configurazione
    WiFiManagerParameter custom_wkey("wkey", "OpenWeather API Key", configManager.data.weatherKey, 64);
    WiFiManagerParameter custom_city("city", "Citta", configManager.data.weatherCity, 32);
    WiFiManagerParameter custom_country("country", "Paese (es. IT)", configManager.data.weatherCountry, 4);
    WiFiManagerParameter custom_tz("tz", "Timezone (es: CET-1CEST...)", configManager.data.timezone, 64);

    wm.addParameter(&custom_wkey);
    wm.addParameter(&custom_city);
    wm.addParameter(&custom_country);
    wm.addParameter(&custom_tz);

    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setConfigPortalTimeout(30); // 30 secondi di timeout per il portale
    wm.setDebugOutput(true);

    // Imposta AP di fallback
    const char* apName = "Termostato_Setup";
    
    // MOSTRA IL QR CODE SUL DISPLAY PRIMA DI BLOCCARE
    ui_show_setup_screen(apName, NULL); 

    // Tenta connessione o avvia AP (bloccante)
    bool res = wm.autoConnect(apName);

    if (!res) {
        Serial.println("Connessione fallita o timeout");
        ui_hide_setup_screen();
        return false;
    }

    // Connesso!
    ui_hide_setup_screen();
    Serial.println("WiFi Connesso!");

    if (shouldSaveConfig) {
        strlcpy(configManager.data.weatherKey, custom_wkey.getValue(), sizeof(configManager.data.weatherKey));
        strlcpy(configManager.data.weatherCity, custom_city.getValue(), sizeof(configManager.data.weatherCity));
        strlcpy(configManager.data.weatherCountry, custom_country.getValue(), sizeof(configManager.data.weatherCountry));
        strlcpy(configManager.data.timezone, custom_tz.getValue(), sizeof(configManager.data.timezone));
        configManager.saveConfig();
    }

    // Applica Timezone
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", configManager.data.timezone, 1);
    tzset();

    return true;
}

void wifi_reset_settings() {
    Serial.println("Resetting WiFi...");
    wm.resetSettings();
    configManager.resetToDefault(); // Opzionale: resetta anche le config app
    delay(1000);
    ESP.restart();
}