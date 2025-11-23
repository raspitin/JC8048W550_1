#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "ui.h"
#include "network_manager.h"
#include "config_manager.h"
#include "web_interface.h"
#include "thermostat.h"

Thermostat thermo;

bool isOnline = false;
unsigned long last_weather_update = 0;
unsigned long last_tick_millis = 0;
bool time_synced = false; 

// Mappa giorni settimana per visualizzazione breve
const char* weekDays[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};

void fetch_weather() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.println("--- Scaricamento Previsioni (Forecast 5 Day) ---");
    
    String city = String(configManager.data.weatherCity); city.trim();
    String country = String(configManager.data.weatherCountry); country.trim();
    String key = String(configManager.data.weatherKey); key.trim();

    HTTPClient http;
    // Endpoint FORECAST invece di WEATHER
    // cnt=40 scarica tutti i segmenti (5 giorni * 8 segmenti/giorno)
    String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + country + "&appid=" + key + "&units=metric&lang=it&cnt=40";

    // Aumentiamo il buffer per il JSON che è grande (~16KB)
    // Su ESP32-S3 con PSRAM non è un problema, ma usiamo un filtro per sicurezza
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        // Usiamo uno stream per non caricare tutto in RAM in una volta se possibile
        WiFiClient *stream = http.getStreamPtr();
        
        // Filtro: ci interessano solo alcuni campi per risparmiare memoria
        StaticJsonDocument<200> filter;
        filter["list"][0]["dt"] = true;
        filter["list"][0]["main"]["temp"] = true;
        filter["list"][0]["weather"][0]["description"] = true;
        filter["list"][0]["weather"][0]["icon"] = true;
        filter["list"][0]["dt_txt"] = true; // Serve per trovare l'ora "12:00:00"

        // Documento dinamico ampio (siamo su S3 con PSRAM, usiamo 32KB per sicurezza)
        DynamicJsonDocument doc(32768); 
        DeserializationError error = deserializeJson(doc, *stream, DeserializationOption::Filter(filter));

        if (!error) {
            JsonArray list = doc["list"];
            int dayIndex = 0;
            bool currentSet = false;

            for (JsonObject item : list) {
                String dt_txt = item["dt_txt"].as<String>();
                
                // 1. Prendi il primissimo elemento come "Meteo Attuale" (approssimazione accettabile)
                if (!currentSet) {
                    float temp = item["main"]["temp"];
                    const char* desc = item["weather"][0]["description"];
                    const char* icon = item["weather"][0]["icon"];
                    String d = String(desc); d[0] = toupper(d[0]);
                    update_current_weather(String(temp, 1), d, String(icon));
                    currentSet = true;
                }

                // 2. Cerca le previsioni per le ore 12:00:00 di ogni giorno successivo
                if (dt_txt.indexOf("12:00:00") >= 0 && dayIndex < 5) {
                    float temp = item["main"]["temp"];
                    const char* icon = item["weather"][0]["icon"];
                    long timestamp = item["dt"];
                    
                    // Calcola giorno settimana
                    time_t ts = (time_t)timestamp;
                    struct tm* timeinfo = localtime(&ts);
                    String dayName = weekDays[timeinfo->tm_wday];

                    update_forecast_item(dayIndex, dayName, String(temp, 0), String(icon));
                    dayIndex++;
                }
            }
            Serial.println("Previsioni aggiornate.");
        } else {
            Serial.print("JSON Error: ");
            Serial.println(error.c_str());
        }
    } else {
        Serial.printf("Err HTTP: %d\n", httpCode);
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    smartdisplay_init();
    create_main_ui(); 
    
    // Init Tick
    last_tick_millis = millis();
    
    if(!configManager.begin()) Serial.println("FS Error");

    isOnline = setup_network();
    
    if (isOnline) {
        configTime(0, 0, "it.pool.ntp.org", "time.nist.gov", "pool.ntp.org");
        setenv("TZ", configManager.data.timezone, 1);
        tzset();

        fetch_weather();
        setup_web_server();
    }
}

void loop() {
    unsigned long current_millis = millis();
    lv_tick_inc(current_millis - last_tick_millis);
    last_tick_millis = current_millis;

    lv_timer_handler();
    
    static unsigned long last_ui_update = 0;
    static unsigned long last_heartbeat = 0;

    if (current_millis - last_ui_update > 500) {
        update_ui(); 
        if (!time_synced) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 0)) { 
                time_synced = true;
            }
        }
        last_ui_update = current_millis;
    }

    if (current_millis - last_heartbeat > 5000) {
        Serial.println("[LOOP] Alive...");
        last_heartbeat = current_millis;
    }

    // Aggiorna previsioni ogni 30 minuti (per non consumare troppe chiamate API)
    if (isOnline && (current_millis - last_weather_update > 1800000)) {
        fetch_weather();
        last_weather_update = current_millis;
    }
    
    delay(5);
}