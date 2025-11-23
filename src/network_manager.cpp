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

// CALLBACK: Viene chiamata SOLO quando entra in modalità AP (Setup)
void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Entrato in modalità AP Configurazione");
    
    // Recupera il nome dell'AP creato
    String ssid = myWiFiManager->getConfigPortalSSID();
    
    // Mostra il QR code e il nome della rete sul display
    ui_show_setup_screen(ssid.c_str(), NULL);
}

bool setup_network() {
    configManager.begin(); 

    // --- SEZIONE RESET ---
    // Se vuoi cancellare le vecchie reti salvate perché bloccato:
    // 1. Decommenta la riga qui sotto (togli //)
    // 2. Carica il codice sull'ESP
    // 3. Aspetta il riavvio (vedrai "Settings erased" su seriale)
    // 4. Ricommenta la riga e ricarica il codice per l'uso normale.
    
    wm.resetSettings(); // <--- TOGLI IL COMMENTO QUI PER RESETTARE
    
    // ---------------------

    // Parametri Custom
    WiFiManagerParameter custom_wkey("wkey", "OpenWeather API Key", configManager.data.weatherKey, 64);
    WiFiManagerParameter custom_city("city", "Citta", configManager.data.weatherCity, 32);
    WiFiManagerParameter custom_country("country", "Paese (es. IT)", configManager.data.weatherCountry, 4);
    WiFiManagerParameter custom_tz("tz", "Timezone", configManager.data.timezone, 64);

    wm.addParameter(&custom_wkey);
    wm.addParameter(&custom_city);
    wm.addParameter(&custom_country);
    wm.addParameter(&custom_tz);

    // Configurazione Callback
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setAPCallback(configModeCallback); // Collega la funzione che mostra il QR
    
    // TIMEOUT FONDAMENTALI
    // 1. ConnectTimeout: Quanto tempo prova a connettersi al router prima di arrendersi e aprire l'AP.
    wm.setConnectTimeout(15); // 15 secondi (risolve il blocco "Tentativo in corso...")

    // 2. ConfigPortalTimeout: Quanto tempo l'AP resta aperto per farti scansionare il QR.
    wm.setConfigPortalTimeout(180); // 3 minuti
    
    wm.setDebugOutput(true);

    // 1. Mostra stato "Connessione..." (Schermata intermedia)
    ui_show_connecting();

    // 2. Avvio automatico
    // - Se ha credenziali valide: si connette entro 15s.
    // - Se fallisce o non ha credenziali: Apre AP, chiama configModeCallback -> Mostra QR.
    const char* apName = "Termostato_Setup";
    bool res = wm.autoConnect(apName);

    if (!res) {
        Serial.println("Timeout Configurazione o Connessione fallita.");
        // Se siamo qui, è scaduto il tempo del portale (3 min) senza successo.
        // Nascondiamo il QR e proseguiamo offline.
        ui_hide_setup_screen();
        return false;
    }

    // Se siamo qui, siamo connessi a Internet
    ui_hide_setup_screen();
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
    Serial.println("Resetting WiFi...");
    wm.resetSettings();
    configManager.resetToDefault(); 
    delay(1000);
    ESP.restart();
}