#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <WiFiUdp.h>
#include <Syslog.h>
#include "ui.h"
#include "network_manager.h"
#include "config_manager.h"
#include "web_interface.h"
#include "thermostat.h"
#include <time.h>      
#include "secrets.h"
#include "config.h"

// Timeout Watchdog (10 secondi)
#define WDT_TIMEOUT 10

Thermostat thermo;
bool isOnline = false;
unsigned long last_weather_update = 0;
unsigned long last_tick_millis = 0;
bool time_synced = false; 

const char* weekDays[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};

// --- SETUP SYSLOG ---
WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_NAME, "app", LOG_KERN);

// Helper per inviare log sia su Seriale che su Syslog
void logMsg(String msg, uint16_t level = LOG_INFO) {
    Serial.println(msg);
    if (isOnline) {
        syslog.log(level, msg);
    }
}

void fetch_weather() {
    if (WiFi.status() != WL_CONNECTED) return;

    // Reset del cane da guardia prima di un'operazione lunga
    esp_task_wdt_reset();

    logMsg("--- Scaricamento Previsioni ---");
    
    String city = String(configManager.data.weatherCity); city.trim();
    String country = String(configManager.data.weatherCountry); country.trim();
    String key = String(configManager.data.weatherKey); key.trim();

    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + country + "&appid=" + key + "&units=metric&lang=it&cnt=40";

    http.setTimeout(8000); // Timeout aumentato a 8s
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        // Scarichiamo tutto in una stringa per evitare timeout dello stream
        String payload = http.getString();
        
        // --- OTTIMIZZAZIONE MEMORIA JSON ---
        // Filtriamo solo i campi necessari per risparmiare RAM
        StaticJsonDocument<512> filter;
        filter["list"][0]["dt"] = true;
        filter["list"][0]["main"]["temp"] = true;
        filter["list"][0]["weather"][0]["description"] = true;
        filter["list"][0]["weather"][0]["icon"] = true;
        filter["list"][0]["dt_txt"] = true;

        // Usiamo un buffer grande (40KB) che su ESP32-S3 finirà in PSRAM
        // evitando di intasare la RAM interna necessaria per il WebServer
        DynamicJsonDocument doc(40960); 

        DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

        if (!error) {
            JsonArray list = doc["list"];
            int dayIndex = 0;
            bool currentSet = false;
            int lastDay = -1;

            for (JsonObject item : list) {
                long timestamp = item["dt"];
                time_t ts = (time_t)timestamp;
                struct tm* timeinfo = localtime(&ts);
                int current_wday = timeinfo->tm_mday;

                // 1. Meteo Corrente (primo elemento della lista)
                if (!currentSet) {
                    float temp = item["main"]["temp"];
                    const char* desc = item["weather"][0]["description"];
                    const char* icon = item["weather"][0]["icon"];
                    String d = String(desc); 
                    if(d.length() > 0) d[0] = toupper(d[0]);

                    update_current_weather(String(temp, 1), d, String(icon));
                    currentSet = true;
                    lastDay = current_wday;
                }

                // 2. Previsioni Giornaliere (ore 11-14 dei giorni successivi)
                if (current_wday != lastDay && timeinfo->tm_hour >= 11 && timeinfo->tm_hour <= 14 && dayIndex < 5) {
                    float temp = item["main"]["temp"];
                    const char* icon = item["weather"][0]["icon"];
                    String dayName = weekDays[timeinfo->tm_wday];

                    update_forecast_item(dayIndex, dayName, String(temp, 0), String(icon));
                    dayIndex++;
                    lastDay = current_wday;
                }
            }
            logMsg("Meteo Aggiornato Correttamente.");
        } else {
            logMsg("Errore JSON: " + String(error.c_str()), LOG_ERR);
        }
    } else {
        logMsg("Errore HTTP: " + String(httpCode), LOG_ERR);
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    // Nessun delay lungo qui per non bloccare
    
    smartdisplay_init();
    ui_init_all(); 
    
    last_tick_millis = millis();
    
    if(!configManager.begin()) Serial.println("Errore FileSystem");

    // Connessione WiFi (Bloccante se non connesso)
    isOnline = setup_network();
    
    if (isOnline) {
        // Inizializza Syslog
        syslog.server(SYSLOG_SERVER, SYSLOG_PORT);
        logMsg("Avvio Sistema. Heap Libero: " + String(ESP.getFreeHeap()));

        configTime(0, 0, "it.pool.ntp.org", "time.nist.gov", "pool.ntp.org");
        setenv("TZ", configManager.data.timezone, 1);
        tzset();

        fetch_weather();
        
        // Piccolo ritardo per permettere al sistema di liberare la memoria del JSON
        // prima di avviare il pesante task del server web
        delay(200); 
        
        setup_web_server();
    }

    // --- ATTIVAZIONE WATCHDOG ALLA FINE ---
    Serial.println("Attivazione Watchdog...");
    esp_task_wdt_init(WDT_TIMEOUT, true); 
    esp_task_wdt_add(NULL); 
}

void loop() {
    // Reset del cane da guardia ad ogni ciclo
    esp_task_wdt_reset();

    unsigned long current_millis = millis();
    lv_tick_inc(current_millis - last_tick_millis);
    last_tick_millis = current_millis;
    
    lv_timer_handler();
    
    static unsigned long last_ui_update = 0;

    // Aggiornamento UI ogni 500ms
    if (current_millis - last_ui_update > 500) {
        update_ui(); 
        
        // Controllo Sync NTP one-shot
        if (!time_synced) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 0)) {
                time_synced = true;
                logMsg("Orario Sincronizzato NTP");
            }
        }
        last_ui_update = current_millis;
    }

    // Aggiornamento Meteo ogni 30 minuti
    if (isOnline && (current_millis - last_weather_update > 1800000)) {
        fetch_weather();
        last_weather_update = current_millis;
    }
    
    delay(5);
}