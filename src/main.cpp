#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "ui.h"
#include "network_manager.h"
#include "config_manager.h"
#include "web_interface.h"
#include "thermostat.h" // <--- REINSERITO

// Definizione istanza globale (necessaria per web_interface.cpp)
Thermostat thermo;      // <--- REINSERITO

bool isOnline = false;
unsigned long last_weather_update = 0;

// Funzione per scaricare il meteo
void fetch_weather() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.println("Aggiornamento meteo...");
    HTTPClient http;
    
    // Costruzione URL OpenWeatherMap
    String url = "http://api.openweathermap.org/data/2.5/weather?q=";
    url += configManager.data.weatherCity;
    url += ",";
    url += configManager.data.weatherCountry;
    url += "&appid=";
    url += configManager.data.weatherKey;
    url += "&units=metric&lang=it";

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            
            // Parsing JSON
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
                float temp = doc["main"]["temp"];
                const char* desc = doc["weather"][0]["description"]; // Es: "cielo sereno"
                
                // Capitalizza la prima lettera
                String descStr = String(desc);
                if(descStr.length() > 0) descStr[0] = toupper(descStr[0]);

                // Aggiorna UI
                update_weather_ui(String(temp, 1), descStr);
                Serial.printf("Meteo: %.1f C, %s\n", temp, descStr.c_str());
            } else {
                Serial.println("Errore JSON Meteo");
            }
        }
    } else {
        Serial.printf("Errore HTTP Meteo: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    Serial.println(">> BOOT START");
    
    // 1. Init Hardware
    smartdisplay_init();
    
    // 2. Crea UI Semplificata (Data/Ora/Meteo)
    create_main_ui();
    
    // 3. Filesystem
    if(!configManager.begin()) Serial.println("FS Error");

    // 4. Network Setup (Bloccante se non configurato)
    isOnline = setup_network();
    
    // 5. Primo aggiornamento meteo e avvio Web Server
    if (isOnline) {
        fetch_weather();
        setup_web_server(); // Ora funzionerà perché 'thermo' esiste
        Serial.println(">> ONLINE");
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
    
    // Nota: Ho rimosso la logica termostato nel loop per rispettare 
    // la tua richiesta di "nient'altro", ma l'oggetto esiste per il web server.
    
    delay(5);
}