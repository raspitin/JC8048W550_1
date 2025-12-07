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
    String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + country + "&appid=" + key + "&units=metric&lang=it&cnt=2";

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
            JsonObject item = doc["list"][0];
            float temp = item["main"]["temp"];
            const char* desc = item["weather"][0]["description"];
            const char* icon = item["weather"][0]["icon"];
            String d = String(desc); 
            if(d.length() > 0) d[0] = toupper(d[0]);
            update_current_weather(String(temp, 1), d, String(icon));
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
    
    smartdisplay_init();
    ui_init_all(); 
    
    last_tick_millis = millis();
    
    if(!configManager.begin()) Serial.println("FS Error");

    isOnline = setup_network();
    
    if (isOnline) {
        syslog.server(SYSLOG_SERVER, SYSLOG_PORT);
        logMsg("Avvio Sistema. Heap: " + String(ESP.getFreeHeap()));
        mqtt_setup(); // <--- Avvia MQTT
        configTime(0, 0, "it.pool.ntp.org", "time.nist.gov", "pool.ntp.org");
        setenv("TZ", configManager.data.timezone, 1);
        tzset();

        // NUOVO: Controllo immediato stato relè all'avvio
        logMsg("Verifica Relè...");
        thermo.checkHeartbeat(true); // Force check
        // NUOVO: Avvia ascolto UDP Discovery
        thermo.setup();

        fetch_weather();
        delay(200); 
        setup_web_server();
    }

    Serial.println("Watchdog Start...");
    esp_task_wdt_init(WDT_TIMEOUT, true); 
    esp_task_wdt_add(NULL); 
}

void loop() {
    esp_task_wdt_reset();

    if (isOnline) {
        mqtt_loop(); // <--- Mantieni vivo MQTT
    }

    // 4. UI UPDATE
    // ... all'interno del timer 500ms o più lento (es. ogni 5 sec per MQTT) ...
    static unsigned long last_mqtt_pub = 0;
    if (millis() - last_mqtt_pub > 5000) { // Pubblica ogni 5 secondi
        mqtt_publish_state(thermo.getCurrentTemp(), thermo.getTarget(), thermo.isHeatingState());
        last_mqtt_pub = millis();
    }

    // 1. GESTIONE TERMOSTATO (Sensor, Logic, Heartbeat ogni 60s)
    thermo.run();

    // 2. GESTIONE WIFI RESILIENCE (Ogni 60 sec)
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
                
                // Al ripristino WiFi, forziamo un controllo relè immediato
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
    
    // 4. UI UPDATE (Ogni 500ms)
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