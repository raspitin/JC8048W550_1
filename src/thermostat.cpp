#include "thermostat.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"
#include <time.h>

// Helper privato per inviare il comando
void sendRelayCommand(bool turnOn) {
    // 1. Controllo Locale (Invariato)
    #ifdef RELAY_PIN
        bool pinState = turnOn;
        if (RELAY_ACTIVE_LOW) pinState = !pinState; 
        digitalWrite(RELAY_PIN, pinState);
    #endif

    // 2. Controllo Remoto (Con Rolling Token)
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        
        // Calcolo del Token basato sul tempo
        time_t now;
        time(&now);
        struct tm *timeinfo = localtime(&now);
        
        // Prendo i minuti (0-59) come indice
        int currentMinute = timeinfo->tm_min;
        const char* token = SECRET_TOKENS[currentMinute];
        
        // Costruisco URL: .../on?auth=ParolaDelMinuto
        String command = turnOn ? "on" : "off";
        String url = String("http://") + REMOTE_RELAY_IP + "/" + command + "?auth=" + token;
        
        Serial.printf("Relay Cmd: %s (Token: %s)\n", command.c_str(), token);
        
        http.setTimeout(500); 
        
        if (http.begin(url)) {
            int httpCode = http.GET();
            if (httpCode == 200) Serial.println("Slave: OK");
            else Serial.printf("Slave Err: %d\n", httpCode);
            http.end();
        }
    }
}

Thermostat::Thermostat() {
    #ifdef RELAY_PIN
        pinMode(RELAY_PIN, OUTPUT);
        // Stato iniziale spento
        bool pinState = false;
        if (RELAY_ACTIVE_LOW) pinState = !pinState;
        digitalWrite(RELAY_PIN, pinState);
    #endif
}

void Thermostat::startHeating() {
    if (!isHeating) {
        isHeating = true;
        Serial.println(">>> CALDAIA ON <<<");
        sendRelayCommand(true);
    }
}

void Thermostat::stopHeating() {
    if (isHeating) {
        isHeating = false;
        Serial.println(">>> CALDAIA OFF <<<");
        sendRelayCommand(false);
    }
}

void Thermostat::update(float currentTemp) {
    this->currentTemp = currentTemp;
    
    // Logica Isteresi semplice
    if (isHeating) {
        // Spegne se supera il target + isteresi
        if (currentTemp >= targetTemp + TEMP_HYSTERESIS) {
            stopHeating();
        }
    } else {
        // Accende se scende sotto target - isteresi
        if (currentTemp <= targetTemp - TEMP_HYSTERESIS) {
            startHeating();
        }
    }
}

void Thermostat::setTarget(float target) {
    this->targetTemp = target;
    // Forza un controllo immediato
    update(this->currentTemp); 
}

float Thermostat::getTarget() { return targetTemp; }
float Thermostat::getCurrentTemp() { return currentTemp; }
bool Thermostat::isHeatingState() { return isHeating; }

float Thermostat::readLocalSensor() {
    // Simulazione (Sostituire con lettura vera DHT/DS18B20)
    static float simTemp = 19.0;
    // Se acceso scalda, se spento raffredda
    if(isHeating) simTemp += 0.005; else simTemp -= 0.005;
    
    // Limiti simulazione
    if (simTemp > 24) simTemp = 24;
    if (simTemp < 15) simTemp = 15;
    
    return simTemp;
}