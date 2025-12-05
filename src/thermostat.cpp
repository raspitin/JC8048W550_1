#include "thermostat.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"
#include "config_manager.h"
#include <time.h>

// Heartbeat ogni 60 secondi per rilevare rapidamente riconnessioni
#define HEARTBEAT_INTERVAL (60 * 1000)
#define SENSOR_READ_INTERVAL 5000 

bool Thermostat::sendRelayCommand(bool turnOn) {
    bool success = false;
    #ifdef RELAY_PIN
        bool pinState = turnOn;
        if (RELAY_ACTIVE_LOW) pinState = !pinState; 
        digitalWrite(RELAY_PIN, pinState);
        success = true; 
    #endif

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        time_t now; time(&now);
        struct tm *timeinfo = localtime(&now);
        int currentMinute = timeinfo->tm_min;
        const char* token = SECRET_TOKENS[currentMinute];
        
        String command = turnOn ? "on" : "off";
        String url = String("http://") + REMOTE_RELAY_IP + "/" + command + "?auth=" + token;
        
        http.setTimeout(2000); 
        if (http.begin(url)) {
            if (http.GET() == 200) { success = true; _relayOnline = true; }
            else { _relayOnline = false; }
            http.end();
        } else { _relayOnline = false; }
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
        if (http.GET() == 200) alive = true;
        http.end();
    }
    _relayOnline = alive;
    // Log su seriale per debug
    if(alive) Serial.println("Heartbeat: OK (Relay Online)");
    else Serial.println("Heartbeat: FAIL (Relay Offline)");
    return alive;
}

// Implementazione con parametro force
void Thermostat::checkHeartbeat(bool force) {
    unsigned long now = millis();
    if (force || (now - _lastHeartbeatTime > HEARTBEAT_INTERVAL)) {
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

void Thermostat::run() {
    unsigned long now = millis();
    // Lettura sensore ogni 5s
    if (now - _lastSensorRead > SENSOR_READ_INTERVAL) {
        float t = readLocalSensor();
        update(t); 
        _lastSensorRead = now;
    }
    // Heartbeat periodico
    checkHeartbeat(false); 
}

bool Thermostat::startHeating() {
    if (sendRelayCommand(true)) {
        isHeating = true;
        Serial.println(">>> CALDAIA ON <<<");
        return true;
    }
    return false;
}

bool Thermostat::stopHeating() {
    if (sendRelayCommand(false)) {
        isHeating = false;
        Serial.println(">>> CALDAIA OFF <<<");
        return true;
    }
    return false;
}

bool Thermostat::startBoost(int minutes) {
    if (startHeating()) {
        time_t now; time(&now);
        _boostEndTime = now + (minutes * 60);
        _boostActive = true;
        targetTemp = TARGET_HEAT_ON; 
        return true;
    }
    _boostActive = false;
    _boostEndTime = 0;
    return false;
}

bool Thermostat::stopBoost() {
    _boostActive = false;
    _boostEndTime = 0;
    update(currentTemp); 
    return _relayOnline;
}

bool Thermostat::isBoostActive() {
    if (!_boostActive) return false;
    time_t now; time(&now);
    if (now >= _boostEndTime) {
        _boostActive = false;
        return false;
    }
    return true;
}

long Thermostat::getBoostRemainingSeconds() {
    if (!isBoostActive()) return 0;
    time_t now; time(&now);
    return (_boostEndTime - now);
}

bool Thermostat::isRelayOnline() {
    return _relayOnline;
}

void Thermostat::update(float currentTemp) {
    this->currentTemp = currentTemp;
    time_t now; time(&now);
    struct tm *timeinfo = localtime(&now);

    if (timeinfo->tm_year + 1900 < 2023) {
        if (isHeating && !isBoostActive()) stopHeating();
        return; 
    }

    int day = timeinfo->tm_wday; 
    int slotIndex = timeinfo->tm_hour * 2 + (timeinfo->tm_min >= 30 ? 1 : 0);
    bool scheduleActive = (configManager.data.weekSchedule[day].timeSlots >> slotIndex) & 1ULL;
    bool boostActive = isBoostActive();

    if (scheduleActive || boostActive) {
        targetTemp = TARGET_HEAT_ON;
        if (!isHeating) startHeating();
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