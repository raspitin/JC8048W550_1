#include "network_manager.h"
#include "config_manager.h"
#include "ui.h"
#include <WiFiManager.h>

WiFiManager wm;
bool shouldSaveConfig = false;

void saveConfigCallback() {
    Serial.println("Salvataggio config richiesto...");
    shouldSaveConfig = true;
}

bool setup_network() {
    configManager.begin(); 

    // Parametri Custom
    WiFiManagerParameter custom_wkey("wkey", "OpenWeather API Key", configManager.data.weatherKey, 64);
    WiFiManagerParameter custom_city("city", "Citta", configManager.data.weatherCity, 32);
    WiFiManagerParameter custom_country("country", "Paese (es. IT)", configManager.data.weatherCountry, 4);
    WiFiManagerParameter custom_tz("tz", "Timezone", configManager.data.timezone, 64);

    wm.addParameter(&custom_wkey);
    wm.addParameter(&custom_city);
    wm.addParameter(&custom_country);
    wm.addParameter(&custom_tz);

    wm.setSaveConfigCallback(saveConfigCallback);
    
    // Timeout aumentato a 180s (3 minuti) per dare tempo di scansionare e connettersi
    wm.setConfigPortalTimeout(180); 
    wm.setDebugOutput(true);

    const char* apName = "Termostato_Setup";
    
    // MOSTRA LA SCHERMATA PRIMA DI CONNETTERE
    Serial.println("Mostro QR Code setup...");
    ui_show_setup_screen(apName, NULL); 
    
    // Un piccolo delay extra per sicurezza, anche se ui_show_setup_screen ora ha un loop interno
    delay(100);

    // Avvia WiFiManager (Bloccante)
    Serial.println("Avvio WiFiManager...");
    bool res = wm.autoConnect(apName);

    if (!res) {
        Serial.println("Connessione fallita o timeout.");
        ui_hide_setup_screen();
        return false;
    }

    ui_hide_setup_screen();
    Serial.println("WiFi Connesso!");

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
    Serial.println("Resetting WiFi...");
    wm.resetSettings();
    configManager.resetToDefault(); 
    delay(1000);
    ESP.restart();
}