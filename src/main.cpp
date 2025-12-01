#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <WiFiUdp.h>       // <--- Necessario per Syslog
#include <Syslog.h>        // <--- Libreria Syslog
#include "ui.h"
#include "network_manager.h"
#include "config_manager.h"
#include "web_interface.h"
#include "thermostat.h"
#include <time.h>      
#include "secrets.h"
#include "config.h"        // Assicurati che includa le define SYSLOG

#define WDT_TIMEOUT 10

Thermostat thermo;
bool isOnline = false;
unsigned long last_weather_update = 0;
unsigned long last_tick_millis = 0;
bool time_synced = false; 

const char* weekDays[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};

// --- SYSLOG SETUP ---
WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_NAME, "app", LOG_KERN);

// Helper per loggare sia su Seriale che su Syslog
void logMsg(String msg, uint16_t level = LOG_INFO) {
    Serial.println(msg);
    if (isOnline) {
        syslog.log(level, msg);
    }
}

void fetch_weather() {
    if (WiFi.status() != WL_CONNECTED) return;
    esp_task_wdt_reset();

    logMsg("--- Scaricamento Previsioni ---"); // Esempio uso logMsg
    
    // ... (resto della funzione fetch_weather invariata) ...
    // Sostituisci i Serial.println critici con logMsg(...) se vuoi vederli remoti
}

void setup() {
    Serial.begin(115200);
    // delay(1000); // Rimosso delay bloccante inutile se non necessario
    
    // --- NON INIZIALIZZARE IL WATCHDOG QUI ---
    // Se lo metti qui, il WiFiManager bloccherà il loop e il WDT resetterà la scheda.
    
    smartdisplay_init();
    ui_init_all(); 
    
    last_tick_millis = millis();
    
    if(!configManager.begin()) Serial.println("FS Error");

    // WiFiManager blocca qui se non connesso
    isOnline = setup_network();
    
    if (isOnline) {
        // Inizializza Syslog se connesso
        syslog.server(SYSLOG_SERVER, SYSLOG_PORT);
        logMsg("Dispositivo Avviato e Connesso! IP: " + WiFi.localIP().toString());

        configTime(0, 0, "it.pool.ntp.org", "time.nist.gov", "pool.ntp.org");
        setenv("TZ", configManager.data.timezone, 1);
        tzset();

        fetch_weather();
        setup_web_server();
    }

    // --- ATTIVA IL WATCHDOG SOLO ALLA FINE ---
    // Ora che il WiFi è su (o abbiamo saltato il setup), possiamo attivare il cane da guardia.
    Serial.println("Attivazione Watchdog...");
    esp_task_wdt_init(WDT_TIMEOUT, true); 
    esp_task_wdt_add(NULL); 
}

void loop() {
    esp_task_wdt_reset();

    unsigned long current_millis = millis();
    lv_tick_inc(current_millis - last_tick_millis);
    last_tick_millis = current_millis;
    
    lv_timer_handler();
    
    static unsigned long last_ui_update = 0;

    if (current_millis - last_ui_update > 500) {
        update_ui(); 
        
        if (!time_synced) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 0)) {
                time_synced = true;
                logMsg("Orario Sincronizzato NTP");
            }
        }
        last_ui_update = current_millis;
    }

    if (isOnline && (current_millis - last_weather_update > 1800000)) {
        fetch_weather();
        last_weather_update = current_millis;
    }
    
    delay(5);
}