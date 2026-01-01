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
#include "influx_manager.h" 

#define WDT_TIMEOUT 10

Thermostat thermo;
bool isOnline = false;
unsigned long last_weather_update = 0;
unsigned long last_tick_millis = 0;
unsigned long last_wifi_check = 0; 
bool time_synced = false; 

// Timer invio dati
unsigned long last_influx_system = 0;
unsigned long last_influx_sensors = 0;

WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_NAME, "app", LOG_KERN);

void logMsg(String msg, uint16_t level = LOG_INFO) {
    Serial.println(msg);
    if (isOnline && WiFi.status() == WL_CONNECTED) {
        syslog.log(level, msg);
    }
}

// ... fetch_weather() rimane uguale ...
void fetch_weather() { /* ... codice invariato ... */ }

void setup() {
    Serial.begin(115200);
    smartdisplay_init();
    ui_show_splash();
    lv_timer_handler(); 
    delay(100);

    if(!configManager.begin()) Serial.println("FS Error");
    ui_init_all(); 
    last_tick_millis = millis();
    isOnline = setup_network();
    
    if (isOnline) {
        syslog.server(SYSLOG_SERVER, SYSLOG_PORT);
        logMsg("Avvio Sistema. Heap: " + String(ESP.getFreeHeap()));
        mqtt_setup(); 
        configTime(0, 0, "it.pool.ntp.org", "time.nist.gov", "pool.ntp.org");
        setenv("TZ", configManager.data.timezone, 1);
        tzset();

        influx.begin();
        influx.reportEvent("system", "boot", "Device restarted");

        logMsg("Verifica Relè...");
        thermo.checkHeartbeat(true); 
        thermo.setup(); 

        fetch_weather();
        delay(200); 
        setup_web_server();
    }
    lv_scr_load(scr_main);
    esp_task_wdt_init(WDT_TIMEOUT, true); 
    esp_task_wdt_add(NULL); 
}

void loop() {
    esp_task_wdt_reset();

    if (isOnline) {
        mqtt_loop(); 
        unsigned long now = millis();

        if (now - last_influx_system > 60000) {
            influx.reportSystemMetrics();
            influx.reportNetworkMetrics(); 
            last_influx_system = now;
        }

        if (now - last_influx_sensors > 30000) {
            // PASSAGGIO PRESSIONE
            influx.reportSensorMetrics(
                thermo.getCurrentTemp(), 
                thermo.getHumidity(), 
                thermo.getPressure(), 
                thermo.getTarget()
            );
            last_influx_sensors = now;
        }
    }

    static unsigned long last_mqtt_pub = 0;
    if (millis() - last_mqtt_pub > 5000) { 
        mqtt_publish_state(thermo.getCurrentTemp(), thermo.getTarget(), thermo.isHeatingState());
        last_mqtt_pub = millis();
    }

    thermo.run();

    static bool lastRelayState = false; 
    bool currentRelayState = thermo.isHeatingState();
    if (currentRelayState != lastRelayState) {
        influx.reportRelayState(currentRelayState, "thermostat_logic");
        lastRelayState = currentRelayState;
    }

    if (millis() - last_wifi_check > 60000) {
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.reconnect();
            isOnline = false;
        } else {
            if (!isOnline) {
                isOnline = true;
                if(!time_synced) configTime(0, 0, "it.pool.ntp.org", "time.nist.gov", "pool.ntp.org");
                thermo.checkHeartbeat(true); 
                influx.begin(); 
            }
        }
        last_wifi_check = millis();
    }

    unsigned long current_millis = millis();
    lv_tick_inc(current_millis - last_tick_millis);
    last_tick_millis = current_millis;
    lv_timer_handler();
    
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

    if (isOnline && (current_millis - last_weather_update > 1800000)) {
        fetch_weather();
        last_weather_update = current_millis;
    }
    delay(5);
}