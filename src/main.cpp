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

// Funzione per scaricare il meteo
void fetch_weather() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("METEO: WiFi non connesso!");
        return;
    }

    Serial.println("--- Avvio Aggiornamento Meteo ---");
    HTTPClient http;
    
    // Trim per sicurezza (rimuove spazi accidentali)
    String city = String(configManager.data.weatherCity);
    String country = String(configManager.data.weatherCountry);
    String apiKey = String(configManager.data.weatherKey);
    city.trim();
    country.trim();
    apiKey.trim();
    
    String url = "http://api.openweathermap.org/data/2.5/weather?q=";
    url += city;
    url += ",";
    url += country;
    url += "&appid=";
    url += apiKey;
    url += "&units=metric&lang=it";

    Serial.printf("URL: %s\n", url.c_str());
    
    http.setTimeout(10000); // Timeout 10 secondi
    http.begin(url);
    
    Serial.println("Invio richiesta HTTP...");
    int httpCode = http.GET();
    Serial.printf("HTTP Code: %d\n", httpCode);

    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.printf("Payload ricevuto (%d bytes)\n", payload.length());
            
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
                float temp = doc["main"]["temp"];
                const char* desc = doc["weather"][0]["description"];
                
                String descStr = String(desc);
                if(descStr.length() > 0) descStr[0] = toupper(descStr[0]);

                update_weather_ui(String(temp, 1), descStr);
                Serial.printf("Meteo OK: %.1f C, %s\n", temp, descStr.c_str());
            } else {
                Serial.printf("Errore JSON: %s\n", error.c_str());
                Serial.println("Payload:");
                Serial.println(payload);
            }
        } else {
            Serial.printf("HTTP Code non OK: %d\n", httpCode);
            String response = http.getString();
            Serial.println("Risposta server:");
            Serial.println(response);
        }
    } else {
        Serial.printf("Errore HTTP: %s (Code: %d)\n", http.errorToString(httpCode).c_str(), httpCode);
    }
    http.end();
    Serial.println("--- Fine Aggiornamento Meteo ---");
}

void setup() {
    Serial.begin(115200);
    Serial.println(">> BOOT START");
    
    // 1. Init Hardware
    smartdisplay_init();
    
    // 2. Crea UI (Data/Ora/Meteo)
    create_main_ui();
    Serial.println("UI: Widget creati.");
    
    // 3. Filesystem
    if(!configManager.begin()) {
        Serial.println("FS Error");
    }

    // 4. Network Setup
    isOnline = setup_network();
    
    // 5. ATTENDI SINCRONIZZAZIONE NTP (CRITICO!)
    if (isOnline) {
        Serial.println("Attendere sincronizzazione NTP...");
        struct tm timeinfo;
        int attempts = 0;
        while(!getLocalTime(&timeinfo) && attempts < 30) {
            delay(500);
            attempts++;
            Serial.print(".");
        }
        
        if(attempts < 30) {
            Serial.println("\n>> ORA SINCRONIZZATA!");
            // Forza primo aggiornamento UI con ora corretta
            update_ui();
        } else {
            Serial.println("\n!! Timeout sincronizzazione NTP");
        }
        
        // 6. Primo aggiornamento meteo
        fetch_weather();
        last_weather_update = millis();
        
        // 7. Avvia Web Server
        setup_web_server();
        Serial.println(">> SISTEMA ONLINE E PRONTO");
    }
}

void loop() {
    lv_timer_handler();
    
    static unsigned long last_ui_update = 0;

    // Aggiorna orologio ogni secondo
    if (millis() - last_ui_update > 1000) {
        update_ui(); 
        last_ui_update = millis();
    }

    // Aggiorna meteo ogni 10 minuti
    if (isOnline && (millis() - last_weather_update > 600000)) {
        fetch_weather();
        last_weather_update = millis();
    }
    
    delay(5);
}