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
bool time_synced = false; 

void fetch_weather() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.println("--- Avvio Aggiornamento Meteo ---");
    
    String city = String(configManager.data.weatherCity);
    city.trim();
    String country = String(configManager.data.weatherCountry);
    country.trim();
    String key = String(configManager.data.weatherKey);
    key.trim();

    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + country + "&appid=" + key + "&units=metric&lang=it";

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            DynamicJsonDocument doc(2048);
            deserializeJson(doc, payload);

            if (!doc.isNull()) {
                float temp = doc["main"]["temp"];
                const char* desc = doc["weather"][0]["description"];
                
                String descStr = String(desc);
                if(descStr.length() > 0) descStr[0] = toupper(descStr[0]);

                update_weather_ui(String(temp, 1), descStr);
                Serial.printf("Meteo OK: %.1f C, %s\n", temp, descStr.c_str());
            }
        } else {
            Serial.printf("ERRORE HTTP Meteo: %d\n", httpCode);
            update_weather_ui("Err", String(httpCode));
        }
    } else {
        Serial.printf("Errore Connessione Meteo: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println(">> BOOT START");
    
    smartdisplay_init();
    create_main_ui(); 
    
    if(!configManager.begin()) Serial.println("FS Error");

    isOnline = setup_network();
    
    if (isOnline) {
        // Server NTP Italiani
        configTime(0, 0, "it.pool.ntp.org", "time.nist.gov", "pool.ntp.org");
        setenv("TZ", configManager.data.timezone, 1);
        tzset();

        fetch_weather();
        setup_web_server();
        Serial.println(">> SISTEMA ONLINE E PRONTO");
    } else {
        Serial.println(">> AVVIO OFFLINE");
    }
}

void loop() {
    lv_timer_handler();
    
    static unsigned long last_ui_update = 0;
    static unsigned long last_heartbeat = 0;

    // Aggiorna UI ogni 500ms
    if (millis() - last_ui_update > 500) {
        update_ui(); 
        
        // Controllo NTP (stampa una volta sola)
        if (!time_synced) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 0)) { 
                time_synced = true;
                Serial.println(">> ORA SINCRONIZZATA!");
            }
        }

        last_ui_update = millis();
    }

    // Heartbeat seriale ogni 5 secondi
    if (millis() - last_heartbeat > 5000) {
        Serial.println("[LOOP] Alive... WiFi Status: " + String(WiFi.status()));
        last_heartbeat = millis();
    }

    // Aggiorna meteo ogni 10 minuti
    if (isOnline && (millis() - last_weather_update > 600000)) {
        fetch_weather();
        last_weather_update = millis();
    }
    
    delay(5);
}