#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "ui.h" // <--- Questo ora contiene le definizioni corrette
#include "network_manager.h"
#include "config_manager.h"
#include "web_interface.h"
#include "thermostat.h"

Thermostat thermo;

bool isOnline = false;
unsigned long last_weather_update = 0;
unsigned long last_tick_millis = 0; // Variabile per il tick LVGL

void fetch_weather() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.println("--- Avvio Aggiornamento Meteo ---");
    HTTPClient http;
    
    String city = String(configManager.data.weatherCity); city.trim();
    String country = String(configManager.data.weatherCountry); country.trim();
    String apiKey = String(configManager.data.weatherKey); apiKey.trim();
    
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + country + "&appid=" + apiKey + "&units=metric&lang=it";

    http.setTimeout(10000);
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
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
            Serial.println("Errore JSON");
        }
    } else {
        Serial.printf("Errore HTTP: %d\n", httpCode);
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Attesa stabilità seriale
    Serial.println(">> BOOT START");
    
    smartdisplay_init();
    create_main_ui(); // Ora questa funzione è riconosciuta grazie a ui.h
    
    last_tick_millis = millis(); // Inizializza tick

    if(!configManager.begin()) Serial.println("FS Error");

    isOnline = setup_network();
    
    if (isOnline) {
        configTime(0, 0, "it.pool.ntp.org", "time.nist.gov", "pool.ntp.org");
        setenv("TZ", configManager.data.timezone, 1);
        tzset();

        fetch_weather();
        setup_web_server();
        Serial.println(">> SISTEMA ONLINE E PRONTO");
    }
}

void loop() {
    // --- GESTIONE TEMPO LVGL (FONDAMENTALE) ---
    unsigned long current_millis = millis();
    lv_tick_inc(current_millis - last_tick_millis);
    last_tick_millis = current_millis;
    // ------------------------------------------

    lv_timer_handler(); // Gestisce la grafica

    static unsigned long last_ui_update = 0;

    // Aggiorna orologio ogni secondo
    if (current_millis - last_ui_update >= 1000) {
        update_ui(); // Ora questa funzione è riconosciuta
        last_ui_update = current_millis;
    }

    // Aggiorna meteo ogni 10 minuti
    if (isOnline && (current_millis - last_weather_update >= 600000)) {
        fetch_weather();
        last_weather_update = current_millis;
    }
    
    delay(5);
}