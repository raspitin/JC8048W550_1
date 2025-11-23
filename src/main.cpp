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
    
    // 1. Init Hardware
    smartdisplay_init();
    create_nest_ui(); 
    
    // 2. Filesystem
    if(!configManager.begin()) Serial.println("Config Init Failed");

    // 3. Network (Gestisce UI e WiFi)
    isOnline = setup_network();
    
    // 4. Web Server
    if (isOnline) {
        // Ritardo critico per permettere a WiFiManager di liberare la porta 80
        Serial.println("Attesa pulizia stack di rete...");
        delay(1000); 
        
        setup_web_server();
        Serial.println(">> WEB SERVER START");
    } else {
        Serial.println(">> OFFLINE MODE");
    }
}

void loop() {
    lv_timer_handler();
    
    static unsigned long last_ui_update = 0;
    static unsigned long last_logic_update = 0;

    if (millis() - last_ui_update > 1000) {
        update_ui(); 
        last_ui_update = millis();
    }

    if (millis() - last_logic_update > 3000) {
        float t = thermo.readLocalSensor();
        thermo.update(t);
        last_logic_update = millis();
    }
    
    delay(5);
}