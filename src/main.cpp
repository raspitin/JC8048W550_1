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
#include "mqtt_manager.h"

#define WDT_TIMEOUT 10

Thermostat thermo;
bool isOnline = false;
unsigned long last_weather_update = 0;
unsigned long last_tick_millis = 0;
unsigned long last_wifi_check = 0; 
bool time_synced = false; 

WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_NAME, "app", LOG_KERN);

void logMsg(String msg, uint16_t level = LOG_INFO) {
    Serial.println(msg);
    if (isOnline && WiFi.status() == WL_CONNECTED) {
        syslog.log(level, msg);
    }
}

void fetch_weather() {
    if (WiFi.status() != WL_CONNECTED) return;
    esp_task_wdt_reset();
    logMsg("--- Scaricamento Meteo ---");
    
    String city = String(configManager.data.weatherCity); city.trim();
    String country = String(configManager.data.weatherCountry); country.trim();
    String key = String(configManager.data.weatherKey); key.trim();

    HTTPClient http;
    // 10 segmenti * 3h = 30 ore di previsione
    String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + country + "&appid=" + key + "&units=metric&lang=it&cnt=10";

    http.setTimeout(5000); 
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        StaticJsonDocument<256> filter;
        filter["list"][0]["main"]["temp"] = true;
        filter["list"][0]["weather"][0]["description"] = true;
        filter["list"][0]["weather"][0]["icon"] = true;

        DynamicJsonDocument doc(10240); 
        DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

        if (!error) {
            // --- DATI OGGI (Indice 0) ---
            JsonObject itemNow = doc["list"][0];
            float tempNow = itemNow["main"]["temp"];
            const char* descNow = itemNow["weather"][0]["description"];
            const char* iconNow = itemNow["weather"][0]["icon"];
            
            String dNow = String(descNow); 
            if(dNow.length() > 0) dNow[0] = toupper(dNow[0]);
            
            update_current_weather(String(tempNow, 1), dNow, String(iconNow));

            // --- DATI DOMANI (Indice 8 = +24h) ---
            if (doc["list"].size() > 8) {
                JsonObject itemTmrw = doc["list"][8];
                float tempTmrw = itemTmrw["main"]["temp"];
                const char* descTmrw = itemTmrw["weather"][0]["description"];
                const char* iconTmrw = itemTmrw["weather"][0]["icon"];
                
                String dTmrw = String(descTmrw); 
                if(dTmrw.length() > 0) dTmrw[0] = toupper(dTmrw[0]);

                update_forecast_item(1, "Domani", String(tempTmrw, 1), dTmrw, String(iconTmrw));
            }

            logMsg("Meteo OK.");
        } else {
            logMsg("JSON Error: " + String(error.c_str()), LOG_ERR);
        }
    } else {
        logMsg("HTTP Err: " + String(httpCode), LOG_ERR);
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    
    // 1. Inizializza Display e LVGL
    smartdisplay_init();

    // 2. Mostra subito la SPLASH SCREEN
    ui_show_splash();
    lv_timer_handler(); // Forza rendering immediato
    delay(100);

    // 3. Carica Configurazione (PRIMA della UI)
    if(!configManager.begin()) Serial.println("FS Error");
    
    // 4. Inizializza il resto della UI (carica Home, Program, ecc. in memoria)
    ui_init_all(); 
    
    last_tick_millis = millis();

    // 5. Configura Rete (Bloccante se non trova WiFi -> Mostra QR Code su Splash)
    isOnline = setup_network();
    
    if (isOnline) {
        syslog.server(SYSLOG_SERVER, SYSLOG_PORT);
        logMsg("Avvio Sistema. Heap: " + String(ESP.getFreeHeap()));
        mqtt_setup(); 
        configTime(0, 0, "it.pool.ntp.org", "time.nist.gov", "pool.ntp.org");
        setenv("TZ", configManager.data.timezone, 1);
        tzset();

        logMsg("Verifica Relè...");
        thermo.checkHeartbeat(true); 
        thermo.setup(); 

        fetch_weather();
        delay(200); 
        setup_web_server();
    }

    // 6. AVVIO COMPLETATO: Passa alla Home Page
    // Nota: scr_main è visibile grazie a 'extern' in ui.h
    lv_scr_load(scr_main);

    Serial.println("Watchdog Start...");
    esp_task_wdt_init(WDT_TIMEOUT, true); 
    esp_task_wdt_add(NULL); 
}

void loop() {
    esp_task_wdt_reset();

    if (isOnline) {
        mqtt_loop(); 
    }

    // MQTT PUBLISH
    static unsigned long last_mqtt_pub = 0;
    if (millis() - last_mqtt_pub > 5000) { 
        mqtt_publish_state(thermo.getCurrentTemp(), thermo.getTarget(), thermo.isHeatingState());
        last_mqtt_pub = millis();
    }

    // 1. GESTIONE TERMOSTATO
    thermo.run();

    // 2. GESTIONE WIFI RESILIENCE
    if (millis() - last_wifi_check > 60000) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi perso! Tento riconnessione...");
            WiFi.reconnect();
            isOnline = false;
        } else {
            if (!isOnline) {
                Serial.println("WiFi Ripristinato!");
                isOnline = true;
                if(!time_synced) configTime(0, 0, "it.pool.ntp.org", "time.nist.gov", "pool.ntp.org");
                thermo.checkHeartbeat(true); 
            }
        }
        last_wifi_check = millis();
    }

    // 3. LVGL TICK
    unsigned long current_millis = millis();
    lv_tick_inc(current_millis - last_tick_millis);
    last_tick_millis = current_millis;
    lv_timer_handler();
    
    // 4. UI UPDATE
    static unsigned long last_ui_update = 0;
    if (current_millis - last_ui_update > 500) {
        update_ui(); 
        
        if (!time_synced) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 0)) {
                time_synced = true;
                logMsg("NTP OK: " + String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min));
            }
        }
        last_ui_update = current_millis;
    }

    // 5. METEO UPDATE
    if (isOnline && (current_millis - last_weather_update > 1800000)) {
        fetch_weather();
        last_weather_update = current_millis;
    }
    
    delay(5);
}