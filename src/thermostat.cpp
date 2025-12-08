#include "thermostat.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"
#include "config_manager.h"
#include <time.h>

#define SENSOR_READ_INTERVAL 5000       // Lettura sensore ogni 5s (Data Logging)
#define CONTROL_CHECK_INTERVAL (15 * 60 * 1000) // Logica termostato ogni 15 min
#define RELAY_TIMEOUT_MS 15000 
#define HEARTBEAT_INTERVAL (60 * 1000)
#define DHT_PIN 17      
#define DHT_TYPE DHT11  

Thermostat::Thermostat() {
    #ifdef RELAY_PIN
    if (RELAY_PIN >= 0) {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
    }
    #endif
    _relayIP.fromString(DEFAULT_REMOTE_RELAY_IP);
    _dht = new DHT(DHT_PIN, DHT_TYPE);
    _dht->begin();
}

void Thermostat::setup() {
    _discoveryUdp.begin(DISCOVERY_PORT);
}

void Thermostat::checkDiscovery() {
    int packetSize = _discoveryUdp.parsePacket();
    if (packetSize) {
        char packetBuffer[255];
        int len = _discoveryUdp.read(packetBuffer, 255);
        if (len > 0) packetBuffer[len] = 0;
        
        if (String(packetBuffer).startsWith(DISCOVERY_PACKET_CONTENT)) {
            IPAddress newIP = _discoveryUdp.remoteIP();
            if (newIP != _relayIP || !_relayOnline) {
                Serial.print("Relè Trovato: ");
                Serial.println(newIP.toString());
            }
            _relayIP = newIP;
            _relayOnline = true;
            _lastPacketTime = millis();
        }
    }

    if (_relayOnline && (millis() - _lastPacketTime > RELAY_TIMEOUT_MS)) {
        Serial.println("Relè Perso (Timeout UDP)");
        _relayOnline = false;
    }
}

void Thermostat::checkHeartbeat(bool force) {
    unsigned long now = millis();
    if (force || (now - _lastHeartbeatTime > HEARTBEAT_INTERVAL)) {
        pingRelay();
        _lastHeartbeatTime = now;
    }
}

bool Thermostat::pingRelay() {
    if (WiFi.status() != WL_CONNECTED) return false;
    String ip = _relayOnline ? _relayIP.toString() : String(DEFAULT_REMOTE_RELAY_IP);
    HTTPClient http;
    String url = String("http://") + ip + "/status";
    http.setTimeout(1500);
    
    bool alive = false;
    if (http.begin(url)) {
        if (http.GET() == 200) alive = true;
        http.end();
    }
    
    if (alive) {
        _relayOnline = true;
        _lastPacketTime = millis(); 
    }
    return alive;
}

bool Thermostat::sendRelayCommand(bool turnOn) {
    bool success = false;
    
    #ifdef RELAY_PIN
    if (RELAY_PIN >= 0) {
        bool pinState = turnOn;
        if (RELAY_ACTIVE_LOW) pinState = !pinState; 
        digitalWrite(RELAY_PIN, pinState);
        success = true; 
    }
    #endif

    if (WiFi.status() == WL_CONNECTED) {
        String ip = _relayOnline ? _relayIP.toString() : String(DEFAULT_REMOTE_RELAY_IP);
        HTTPClient http;
        time_t now; time(&now);
        struct tm *timeinfo = localtime(&now);
        int currentMinute = timeinfo->tm_min;
        const char* token = SECRET_TOKENS[currentMinute];
        
        String command = turnOn ? "on" : "off";
        String url = "http://" + ip + "/" + command + "?auth=" + token;
        
        http.setTimeout(1500); 
        if (http.begin(url)) {
            if (http.GET() == 200) {
                success = true;
                _relayOnline = true;
                _lastPacketTime = millis();
            }
            http.end();
        }
    } 
    return success;
}

void Thermostat::run() {
    if (WiFi.status() == WL_CONNECTED) {
        checkDiscovery();
    }

    unsigned long now = millis();

    // 1. LETTURA SENSORI AD ALTA FREQUENZA (ogni 5 sec)
    // Questo serve per avere dati freschi sulla UI e per il futuro InfluxDB
    if (now - _lastSensorRead > SENSOR_READ_INTERVAL) {
        float t = readLocalSensor();
        this->currentTemp = t; // Aggiorno lo stato interno
        _lastSensorRead = now;

        // TODO: Qui inseriremo la chiamata a InfluxDB: writeToInflux(t, currentHumidity);
    }
    
    // 2. LOGICA TERMOSTATO A BASSA FREQUENZA (ogni 15 min)
    // Evita di stressare la caldaia. Esegue check orari e isteresi.
    // Viene eseguito immediatamente al primo avvio (_firstRun).
    if (_firstRun || (now - _lastControlTime > CONTROL_CHECK_INTERVAL)) {
        update(this->currentTemp); 
        _lastControlTime = now;
        _firstRun = false;
        // Serial.println("Controllo Termostato (Ciclo 15min) eseguito.");
    }
    
    checkHeartbeat(false);
}

bool Thermostat::isRelayOnline() { return _relayOnline; }
String Thermostat::getRelayIP() { return _relayIP.toString(); }

bool Thermostat::startHeating() {
    if (sendRelayCommand(true)) {
        isHeating = true;
        return true;
    }
    return false;
}

bool Thermostat::stopHeating() {
    if (sendRelayCommand(false)) {
        isHeating = false;
        return true;
    }
    return false;
}

// --- LOGICA BOOST ---
bool Thermostat::startBoost(int minutes) {
    _manualOverride = false;
    time_t now; time(&now);
    _boostEndTime = now + (minutes * 60);
    _boostActive = true;
    targetTemp = TARGET_HEAT_ON; 
    
    // IMPORTANTE: Le azioni manuali (Boost, Override, SetTarget) devono
    // scavalcare il timer di 15 minuti e agire subito.
    update(currentTemp); 
    return true;
}

bool Thermostat::stopBoost() {
    _boostActive = false;
    _boostEndTime = 0;
    update(currentTemp); // Aggiornamento immediato
    return true;
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

// --- LOGICA OVERRIDE (CHE CALDO) ---
void Thermostat::toggleOverride() {
    if (isBoostActive()) {
        stopBoost();
        return;
    }
    _manualOverride = !_manualOverride;
    update(currentTemp); // Aggiornamento immediato
}

bool Thermostat::isOverrideActive() {
    return _manualOverride;
}

// --- CUORE LOGICA TERMOSTATO ---
void Thermostat::update(float currentTemp) {
    this->currentTemp = currentTemp;
    time_t now; time(&now);
    struct tm *timeinfo = localtime(&now);

    if (timeinfo->tm_year + 1900 < 2023) {
        if (isHeating) stopHeating();
        return; 
    }

    int day = timeinfo->tm_wday; 
    int slotIndex = timeinfo->tm_hour * 2 + (timeinfo->tm_min >= 30 ? 1 : 0);

    bool slotChanged = (slotIndex != _lastScheduleSlot);
    
    if (slotChanged) {
        _manualOverride = false; 
        _lastScheduleSlot = slotIndex;
        
        bool scheduleActive = (configManager.data.weekSchedule[day].timeSlots >> slotIndex) & 1ULL;
        if (scheduleActive) targetTemp = TARGET_HEAT_ON; 
        else targetTemp = TARGET_HEAT_OFF; 
    }

    // 2. GESTIONE BOOST E OVERRIDE
    if (isBoostActive()) {
        targetTemp = TARGET_HEAT_ON;
    }
    else if (_manualOverride) {
        targetTemp = TARGET_HEAT_OFF;
    }

    // 3. LOGICA DI CONTROLLO (ISTERESI)
    if (currentTemp < (targetTemp - TEMP_HYSTERESIS)) {
        if (!isHeating) startHeating();
    } 
    else if (currentTemp > (targetTemp + TEMP_HYSTERESIS)) {
        if (isHeating) stopHeating();
    }
}

// Permette alla UI di cambiare il target temporaneamente
void Thermostat::setTarget(float target) { 
    this->targetTemp = target; 
    update(this->currentTemp); // Aggiornamento immediato se cambiato da UI
}

float Thermostat::getTarget() { return targetTemp; }
float Thermostat::getCurrentTemp() { return currentTemp; }
bool Thermostat::isHeatingState() { return isHeating; }

float Thermostat::readLocalSensor() {
    float t = _dht->readTemperature();
    float h = _dht->readHumidity();

    if (isnan(t) || isnan(h)) {
        Serial.println("Errore lettura DHT!");
        return this->currentTemp; 
    }
    this->currentHumidity = h;
    return t;
}

float Thermostat::getHumidity() {
    return this->currentHumidity;
}