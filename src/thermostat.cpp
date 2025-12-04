#include "thermostat.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"
#include "config_manager.h"
#include <time.h>

// Heartbeat ogni 20 minuti (in millisecondi)
#define HEARTBEAT_INTERVAL (20 * 60 * 1000)

bool Thermostat::sendRelayCommand(bool turnOn) {
    bool success = false;

    // 1. Relè Locale (Sempre OK se definito)
    #ifdef RELAY_PIN
        bool pinState = turnOn;
        if (RELAY_ACTIVE_LOW) pinState = !pinState; 
        digitalWrite(RELAY_PIN, pinState);
        success = true; 
    #endif

    // 2. Relè Remoto (WiFi)
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        time_t now;
        time(&now);
        struct tm *timeinfo = localtime(&now);
        int currentMinute = timeinfo->tm_min;
        const char* token = SECRET_TOKENS[currentMinute];
        
        String command = turnOn ? "on" : "off";
        String url = String("http://") + REMOTE_RELAY_IP + "/" + command + "?auth=" + token;
        
        // Timeout breve (2s) per non bloccare l'interfaccia in caso di errore
        http.setTimeout(2000); 
        
        if (http.begin(url)) {
            int httpCode = http.GET();
            if (httpCode == 200) {
                success = true;
                _relayOnline = true;
            } else {
                Serial.printf("Relay Error: %d\n", httpCode);
                _relayOnline = false;
            }
            http.end();
        } else {
            Serial.println("Relay Connection Failed");
            _relayOnline = false;
        }
    } 
    
    return success;
}

bool Thermostat::pingRelay() {
    if (WiFi.status() != WL_CONNECTED) return false;
    
    HTTPClient http;
    String url = String("http://") + REMOTE_RELAY_IP + "/status";
    http.setTimeout(2000);
    
    bool alive = false;
    if (http.begin(url)) {
        int code = http.GET();
        if (code == 200) alive = true;
        http.end();
    }
    
    if(alive) {
        Serial.println("Heartbeat: OK");
        _relayOnline = true;
    } else {
        Serial.println("Heartbeat: PERSO");
        _relayOnline = false;
    }
    return alive;
}

void Thermostat::checkHeartbeat() {
    unsigned long now = millis();
    if (now - _lastHeartbeatTime > HEARTBEAT_INTERVAL) {
        pingRelay();
        _lastHeartbeatTime = now;
    }
}

Thermostat::Thermostat() {
    #ifdef RELAY_PIN
        pinMode(RELAY_PIN, OUTPUT);
        bool pinState = false;
        if (RELAY_ACTIVE_LOW) pinState = !pinState;
        digitalWrite(RELAY_PIN, pinState);
    #endif
}

bool Thermostat::startHeating() {
    if (!isHeating) {
        isHeating = true;
        Serial.println(">>> CALDAIA ON <<<");
        return sendRelayCommand(true);
    }
    return true; // Era già accesa
}

bool Thermostat::stopHeating() {
    if (isHeating) {
        isHeating = false;
        Serial.println(">>> CALDAIA OFF <<<");
        return sendRelayCommand(false);
    }
    return true; // Era già spenta
}

// --- BOOST ---
bool Thermostat::startBoost(int minutes) {
    time_t now;
    time(&now);
    _boostEndTime = now + (minutes * 60);
    _boostActive = true;
    
    targetTemp = TARGET_HEAT_ON; 
    return startHeating();
}

bool Thermostat::stopBoost() {
    _boostActive = false;
    _boostEndTime = 0;
    
    // Ricalcola stato (potrebbe dover restare accesa per programma)
    update(currentTemp);
    
    // Ritorniamo true se il relè risponde (in base all'ultimo comando o heartbeat)
    return _relayOnline;
}

bool Thermostat::isBoostActive() {
    if (!_boostActive) return false;
    time_t now;
    time(&now);
    if (now >= _boostEndTime) {
        _boostActive = false;
        return false;
    }
    return true;
}

long Thermostat::getBoostRemainingSeconds() {
    if (!isBoostActive()) return 0;
    time_t now;
    time(&now);
    return (_boostEndTime - now);
}

void Thermostat::update(float currentTemp) {
    this->currentTemp = currentTemp;
    
    time_t now; time(&now);
    struct tm *timeinfo = localtime(&now);

    int day = timeinfo->tm_wday; 
    int slotIndex = timeinfo->tm_hour * 2 + (timeinfo->tm_min >= 30 ? 1 : 0);
    bool scheduleActive = (configManager.data.weekSchedule[day].timeSlots >> slotIndex) & 1ULL;
    bool boostActive = isBoostActive();

    bool shouldHeat = scheduleActive || boostActive;

    if (shouldHeat) {
        targetTemp = TARGET_HEAT_ON;
        if (!isHeating) startHeating(); // Qui è automatico, ignoriamo errore per ora
    } else {
        targetTemp = TARGET_HEAT_OFF;
        if (isHeating) stopHeating();
    }
}

void Thermostat::setTarget(float target) { this->targetTemp = target; }
float Thermostat::getTarget() { return targetTemp; }
float Thermostat::getCurrentTemp() { return currentTemp; }
bool Thermostat::isHeatingState() { return isHeating; }

float Thermostat::readLocalSensor() {
    static float simTemp = 19.0;
    if(isHeating) simTemp += 0.005; else simTemp -= 0.005;
    if (simTemp > 24) simTemp = 24;
    if (simTemp < 15) simTemp = 15;
    return simTemp;
}