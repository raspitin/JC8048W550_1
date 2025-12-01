#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h> // Watchdog
#include "ui.h"
#include "network_manager.h"
#include "config_manager.h"
#include "web_interface.h"
#include "thermostat.h"
#include <time.h>      
#include "secrets.h"

// Configurazione Watchdog: 10 secondi di timeout
#define WDT_TIMEOUT 10

Thermostat thermo;

bool isOnline = false;
unsigned long last_weather_update = 0;
unsigned long last_tick_millis = 0;
bool time_synced = false; 

const char* weekDays[] = {"DOM", "LUN", "MAR", "MER", "GIO", "VEN", "SAB"};

void fetch_weather() {
    if (WiFi.status() != WL_CONNECTED) return;

    // Reset WDT anche qui per operazioni lunghe
    esp_task_wdt_reset();

    Serial.println("--- Scaricamento Previsioni ---");
    
    String city = String(configManager.data.weatherCity); city.trim();
    String country = String(configManager.data.weatherCountry); country.trim();
    String key = String(configManager.data.weatherKey); key.trim();

    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + country + "&appid=" + key + "&units=metric&lang=it&cnt=40";

    http.setTimeout(5000); // Timeout HTTP ridotto a 5s per non bloccare troppo
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        WiFiClient *stream = http.getStreamPtr();
        
        StaticJsonDocument<200> filter;
        filter["list"][0]["dt"] = true;
        filter["list"][0]["main"]["temp"] = true;
        filter["list"][0]["weather"][0]["description"] = true;
        filter["list"][0]["weather"][0]["icon"] = true;
        filter["list"][0]["dt_txt"] = true;

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

                if (current_wday != lastDay && timeinfo->tm_hour >= 11 && timeinfo->tm_hour <= 14 && dayIndex < 5) {
                    float temp = item["main"]["temp"];
                    const char* icon = item["weather"][0]["icon"];
                    String dayName = weekDays[timeinfo->tm_wday];

                    update_forecast_item(dayIndex, dayName, String(temp, 0), String(icon));
                    dayIndex++;
                    lastDay = current_wday;
                }
            }
            Serial.println("Meteo Aggiornato.");
        } else {
            Serial.print("JSON Error: "); Serial.println(error.c_str());
        }
    } else {
        Serial.printf("Err HTTP: %d\n", httpCode);
    }
    http.end();
    
    // Reset WDT post download
    esp_task_wdt_reset();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // 1. Inizializza Watchdog
    esp_task_wdt_init(WDT_TIMEOUT, true); // Panic = true (riavvia)
    esp_task_wdt_add(NULL); // Aggiungi il thread corrente (loop)
    
    smartdisplay_init();
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
    // Nutri il cane da guardia!
    esp_task_wdt_reset();

    unsigned long current_millis = millis();
    lv_tick_inc(current_millis - last_tick_millis);
    last_tick_millis = current_millis;
    
    lv_timer_handler();
    
    static unsigned long last_ui_update = 0;

    if (current_millis - last_ui_update > 500) {
        update_ui(); 
        
        // Sync NTP check one-shot
        if (!time_synced) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 0)) time_synced = true;
        }
        last_ui_update = current_millis;
    }

    if (isOnline && (current_millis - last_weather_update > 1800000)) {
        fetch_weather();
        last_weather_update = current_millis;
    }
    
    delay(5);
}