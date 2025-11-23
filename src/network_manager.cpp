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

// CALLBACK CRITICA
void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("!!! CALLBACK AP ATTIVATA !!!");
    Serial.print("SSID AP Creato: ");
    Serial.println(myWiFiManager->getConfigPortalSSID());
    Serial.print("IP AP: ");
    Serial.println(WiFi.softAPIP());
    
    // Recupera il nome dell'AP
    String ssid = myWiFiManager->getConfigPortalSSID();
    
    // Chiama la UI per disegnare il QR
    ui_show_setup_screen(ssid.c_str(), NULL);
}

bool setup_network() {
    configManager.begin(); 

    // --- BLOCCO RESET ---
    //wm.resetSettings();  
    // --------------------

    // Parametri Custom
    WiFiManagerParameter custom_wkey("wkey", "OpenWeather API Key", configManager.data.weatherKey, 64);
    WiFiManagerParameter custom_city("city", "Citta", configManager.data.weatherCity, 32);
    WiFiManagerParameter custom_country("country", "Paese (es. IT)", configManager.data.weatherCountry, 4);
    WiFiManagerParameter custom_tz("tz", "Timezone", configManager.data.timezone, 64);

    wm.addParameter(&custom_wkey);
    wm.addParameter(&custom_city);
    wm.addParameter(&custom_country);
    wm.addParameter(&custom_tz);

    // Configurazione
    wm.setSaveConfigCallback(saveConfigCallback);
    
    // Registra la callback per mostrare il QR
    wm.setAPCallback(configModeCallback); 
    
    wm.setConnectTimeout(15); // Timeout connessione router
    wm.setConfigPortalTimeout(180); // Timeout pagina config
    
    wm.setDebugOutput(true);

    // 1. Mostra UI "Tentativo..."
    Serial.println("Network: Inizio fase connessione...");
    ui_show_connecting();

    // 2. Avvia
    const char* apName = "Termostato_Setup";
    
    // Se la connessione fallisce, WiFiManager chiama configModeCallback internamente
    // e POI blocca l'esecuzione per gestire il server web.
    bool res = wm.autoConnect(apName);

    if (!res) {
        Serial.println("Network: Timeout o errore connessione.");
        ui_hide_setup_screen();
        return false; 
    }

    // Connesso
    ui_hide_setup_screen();
    Serial.println("Network: WiFi Connesso!");
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
    Serial.println("Resetting WiFi...");
    wm.resetSettings();
    configManager.resetToDefault(); 
    delay(1000);
    ESP.restart();
}