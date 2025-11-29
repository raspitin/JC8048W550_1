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

// Mappa giorni settimana
const char* weekDays[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};

void fetch_weather() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.println("--- Scaricamento Previsioni (Forecast) ---");
    
    String city = String(configManager.data.weatherCity); city.trim();
    String country = String(configManager.data.weatherCountry); country.trim();
    String key = String(configManager.data.weatherKey); key.trim();

    HTTPClient http;
    // API Forecast (5 giorni, step di 3 ore)
    // cnt=40 scarica circa 5 giorni completi
    String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + country + "&appid=" + key + "&units=metric&lang=it&cnt=40";

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        WiFiClient *stream = http.getStreamPtr();
        
        // Filtro JSON per risparmiare memoria (il JSON completo è enorme)
        StaticJsonDocument<200> filter;
        filter["list"][0]["dt"] = true;
        filter["list"][0]["main"]["temp"] = true;
        filter["list"][0]["weather"][0]["description"] = true;
        filter["list"][0]["weather"][0]["icon"] = true;
        filter["list"][0]["dt_txt"] = true;

        // Buffer aumentato per gestire la lista
        DynamicJsonDocument doc(32768); 
        DeserializationError error = deserializeJson(doc, *stream, DeserializationOption::Filter(filter));

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

                // 1. Meteo Attuale (Prendiamo il primo slot disponibile come "adesso")
                if (!currentSet) {
                    float temp = item["main"]["temp"];
                    const char* desc = item["weather"][0]["description"];
                    const char* icon = item["weather"][0]["icon"];
                    String d = String(desc); 
                    if(d.length() > 0) d[0] = toupper(d[0]); // Capitalizza

                    // Chiama la funzione corretta della nuova UI
                    update_current_weather(String(temp, 1), d, String(icon));
                    
                    currentSet = true;
                    lastDay = current_wday; // Segna oggi come fatto
                }

                // 2. Previsioni Giorni Successivi
                // Cerchiamo uno slot attorno a mezzogiorno (12:00) per ogni giorno diverso da oggi
                // dt_txt è utile ma costoso, usiamo l'ora dalla struct tm
                if (current_wday != lastDay && timeinfo->tm_hour >= 11 && timeinfo->tm_hour <= 14 && dayIndex < 5) {
                    float temp = item["main"]["temp"];
                    const char* icon = item["weather"][0]["icon"];
                    String dayName = weekDays[timeinfo->tm_wday];

                    update_forecast_item(dayIndex, dayName, String(temp, 0), String(icon));
                    
                    dayIndex++;
                    lastDay = current_wday; // Passa al prossimo giorno
                }
            }
            Serial.println("Meteo aggiornato con successo.");
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
    
    // --- FIX ERRORE: Chiama la funzione corretta ---
    ui_init_all(); 
    
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
    // Gestione Tempo LVGL
    unsigned long current_millis = millis();
    lv_tick_inc(current_millis - last_tick_millis);
    last_tick_millis = current_millis;
    lv_timer_handler();
    
    static unsigned long last_ui_update = 0;
    static unsigned long last_heartbeat = 0;

    // Aggiorna UI
    if (current_millis - last_ui_update > 500) {
        update_ui(); 
        if (!time_synced) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 0)) time_synced = true;
        }
        last_ui_update = current_millis;
    }

    // Aggiorna meteo ogni 30 minuti (per non consumare troppe chiamate API con il Forecast)
    if (isOnline && (current_millis - last_weather_update > 1800000)) {
        fetch_weather();
        last_weather_update = current_millis;
    }
    
    delay(5);
}