#include <Arduino.h>
#include <esp32_smartdisplay.h>
#include "ui.h"
#include "network_manager.h"
#include "config_manager.h"
#include "thermostat.h"
#include "web_interface.h"

Thermostat thermo;
bool isOnline = false;

void setup() {
    Serial.begin(115200);
    Serial.println(">> BOOT START");
    
    // 1. Hardware & Display Init
    smartdisplay_init();
    
    // 2. UI Creation
    create_nest_ui(); 
    
    // 3. Filesystem Init
    if(!configManager.begin()) {
        Serial.println("Config Manager Init Failed");
    }

    // 4. Network Setup (Blocking if not configured)
    isOnline = setup_network();
    
    // 5. Web Server Init
    if (isOnline) {
        setup_web_server();
        Serial.println(">> WEB SERVER START");
    } else {
        Serial.println(">> OFFLINE MODE");
    }
}

void loop() {
    // LVGL Tick
    lv_timer_handler();
    
    // Application Logic
    static unsigned long last_ui_update = 0;
    static unsigned long last_logic_update = 0;

    // UI Refresh (1 sec)
    if (millis() - last_ui_update > 1000) {
        update_ui(); 
        last_ui_update = millis();
    }

    // Logic Refresh (3 sec)
    if (millis() - last_logic_update > 3000) {
        float t = thermo.readLocalSensor();
        thermo.update(t);
        last_logic_update = millis();
    }
    
    delay(5);
}