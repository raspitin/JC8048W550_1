#include "network_manager.h"
#include "config_manager.h"
#include "ui.h"
#include <WiFiManager.h>
#include <lvgl.h> // Serve per forzare l'aggiornamento UI qui

// Flag globale per il salvataggio
bool shouldSaveConfig = false;

void saveConfigCallback() {
    Serial.println("Salvataggio config richiesto...");
    shouldSaveConfig = true;
}

// Callback modalità AP
void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Entrato in modalità AP Configurazione");
    String ssid = myWiFiManager->getConfigPortalSSID();
    ui_show_setup_screen(ssid.c_str(), NULL);
}

bool setup_network() {
    configManager.begin(); 
    
    // --- ISTANZA LOCALE (IMPORTANTE!) ---
    // Creandola qui, verrà distrutta alla fine della funzione,
    // liberando la porta 80 per il tuo Web Server principale.
    WiFiManager wm;

    // Parametri Custom
    WiFiManagerParameter custom_wkey("wkey", "OpenWeather API Key", configManager.data.weatherKey, 64);
    WiFiManagerParameter custom_city("city", "Citta", configManager.data.weatherCity, 32);
    WiFiManagerParameter custom_country("country", "Paese (es. IT)", configManager.data.weatherCountry, 4);
    WiFiManagerParameter custom_tz("tz", "Timezone", configManager.data.timezone, 64);

    wm.addParameter(&custom_wkey);
    wm.addParameter(&custom_city);
    wm.addParameter(&custom_country);
    wm.addParameter(&custom_tz);

    // Configurazione Callback e Timeout
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setAPCallback(configModeCallback);
    wm.setConnectTimeout(20);       // 20 sec per provare a connettersi al router
    wm.setConfigPortalTimeout(180); // 3 min per il QR code
    wm.setDebugOutput(true);

    // 1. Mostra UI Attesa
    ui_show_connecting();

    // 2. Avvio (Bloccante)
    const char* apName = "Termostato_Setup";
    bool res = wm.autoConnect(apName);

    if (!res) {
        Serial.println("Network: Fallito.");
        ui_hide_setup_screen();
        return false;
    }

    // 3. Connesso!
    Serial.println("Network: WiFi Connesso!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // Nascondi QR e forza aggiornamento display IMMEDIATO
    ui_hide_setup_screen();
    
    // Forziamo LVGL a ridisegnare subito per togliere il QR code
    // prima che il codice prosegua
    for(int i=0; i<10; i++) {
        lv_timer_handler();
        delay(10);
    }

    // Salvataggio parametri
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

    // Qui wm viene distrutto e la porta 80 liberata
    return true;
}

void wifi_reset_settings() {
    // Anche qui istanza locale
    WiFiManager wm;
    Serial.println("Resetting WiFi...");
    wm.resetSettings();
    configManager.resetToDefault(); 
    delay(1000);
    ESP.restart();
}