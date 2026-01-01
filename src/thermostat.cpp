#include "thermostat.h"
#include "config.h"
#include "config_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"
#include <time.h>

#define SENSOR_READ_INTERVAL 2000       // Lettura sensore ogni 2s (stabilità I2C)
#define RELAY_TIMEOUT_MS 15000 
#define HEARTBEAT_INTERVAL (60 * 1000)

// Configurazione I2C per JC8048W550 (Connettore P4)
#define I2C_SDA_PIN 17  
#define I2C_SCL_PIN 18

Thermostat::Thermostat() {
    #ifdef RELAY_PIN
    if (RELAY_PIN >= 0) {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
    }
    #endif
    
    // Default IP (fallback)
    _relayIP.fromString(DEFAULT_REMOTE_RELAY_IP);

    // Inizializzazione variabili cache
    last_read_time = 0;
    cached_temp = 0.0;
    cached_hum = 0.0;
    cached_press = 0.0;
}

void Thermostat::setup() {
    _discoveryUdp.begin(DISCOVERY_PORT);

    // --- SETUP SENSORI I2C ---
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    delay(100); // Wait for sensors power up

    bool aht_ok = aht.begin();
    // BMP280 address is usually 0x76 or 0x77
    bool bmp_ok = bmp.begin(0x76);
    if (!bmp_ok) bmp_ok = bmp.begin(0x77);

    if (aht_ok && bmp_ok) {
        Serial.println("Sensori AHT20 e BMP280 OK!");
        sensorsReady = true;
    } else {
        Serial.println("ERRORE SENSORI I2C!");
        if(!aht_ok) Serial.println("- AHT20 mancante");
        if(!bmp_ok) Serial.println("- BMP280 mancante");
        sensorsReady = false;
    }
    
    // Prima lettura immediata
    readSensors();
}

void Thermostat::readSensors() {
    if (!sensorsReady) return;
    
    unsigned long now = millis();
    if (now - last_read_time < SENSOR_READ_INTERVAL && last_read_time > 0) return;

    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp); // Popola gli oggetti evento

    // Aggiorna cache
    cached_temp = temp.temperature;
    cached_hum = humidity.relative_humidity;
    cached_press = bmp.readPressure() / 100.0F; // Pa -> hPa

    // Aggiorna le variabili membro usate dalla logica del termostato
    this->currentTemp = cached_temp + configManager.data.tempOffset; // Calibrazione
    this->currentHumidity = cached_hum;

    last_read_time = now;
}

// Getters che forzano la lettura se necessario
float Thermostat::getCurrentTemp() {
    readSensors();
    if (isnan(this->currentTemp)) return 0.0;
    return this->currentTemp;
}

float Thermostat::getHumidity() {
    readSensors();
    if (isnan(this->currentHumidity)) return 0.0;
    return this->currentHumidity;
}

float Thermostat::getPressure() {
    readSensors();
    if (isnan(cached_press)) return 0.0;
    return cached_press;
}

// -------------------------------------------------------------------------
// LOGICA TERMOSTATO (Discovery, Boost, Run) - Preservata
// -------------------------------------------------------------------------

void Thermostat::checkDiscovery() {
    int packetSize = _discoveryUdp.parsePacket();
    if (packetSize) {
        char packetBuffer[255];
        int len = _discoveryUdp.read(packetBuffer, 255);
        if (len > 0) packetBuffer[len] = 0;
        
        String msg = String(packetBuffer);
        if (msg.startsWith("RELAY_HERE_V1")) {
            _relayIP = _discoveryUdp.remoteIP();
            _relayOnline = true;
            _lastHeartbeat = millis();
        }
    }
}

void Thermostat::sendDiscovery() {
    _discoveryUdp.beginPacket(IPAddress(255,255,255,255), DISCOVERY_PORT);
    _discoveryUdp.print(DISCOVERY_PACKET_CONTENT);
    _discoveryUdp.endPacket();
}

void Thermostat::checkHeartbeat(bool force) {
    if (force || millis() - _lastHeartbeat > HEARTBEAT_INTERVAL) {
        sendDiscovery();
    }
    checkDiscovery();
    if (millis() - _lastHeartbeat > RELAY_TIMEOUT_MS) {
        _relayOnline = false;
    }
}

bool Thermostat::isRelayOnline() { return _relayOnline; }
String Thermostat::getRelayIP() { return _relayIP.toString(); }

bool Thermostat::startBoost(int minutes) {
    _boostActive = true;
    _boostEndTime = time(NULL) + (minutes * 60);
    return true;
}

bool Thermostat::stopBoost() {
    _boostActive = false;
    _boostEndTime = 0;
    return true;
}

bool Thermostat::isBoostActive() {
    if (_boostActive && time(NULL) > _boostEndTime) {
        _boostActive = false; 
    }
    return _boostActive;
}

long Thermostat::getBoostRemainingSeconds() {
    if (!isBoostActive()) return 0;
    return (long)difftime(_boostEndTime, time(NULL));
}

void Thermostat::toggleOverride() {
    _manualOverride = !_manualOverride;
    // Se attiviamo override manuale (OFF forzato), spegniamo boost
    if (_manualOverride) stopBoost();
}

bool Thermostat::isOverrideActive() { return _manualOverride; }

bool Thermostat::startHeating() {
    isHeating = true;
    // Hardware locale
    #ifdef RELAY_PIN
    if (RELAY_PIN >= 0) digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? LOW : HIGH);
    #endif
    return true;
}

bool Thermostat::stopHeating() {
    isHeating = false;
    #ifdef RELAY_PIN
    if (RELAY_PIN >= 0) digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
    #endif
    return true;
}

// Logica per calcolare il target attuale in base alla schedule
float Thermostat::getScheduledTarget() {
    // Se non abbiamo l'ora, fallback su default (Economy)
    time_t now; 
    time(&now);
    struct tm timeinfo;
    if(!localtime_r(&now, &timeinfo)) {
        return TARGET_HEAT_OFF;
    }

    int day = (timeinfo.tm_wday + 6) % 7; // Lun=0 ... Dom=6
    int hour = timeinfo.tm_hour;
    int min = timeinfo.tm_min;
    
    // Slot index (0-47)
    int slotIndex = hour * 2 + (min >= 30 ? 1 : 0);
    
    // Leggi bitmask
    bool isComfort = (configManager.data.weekSchedule[day].timeSlots >> slotIndex) & 1ULL;
    
    return isComfort ? TARGET_HEAT_ON : TARGET_HEAT_OFF;
}

void Thermostat::run() {
    // Aggiorna sensori
    readSensors(); 
    checkHeartbeat(); 

    // 1. DETERMINA TARGET TEMP
    if (!_manualOverride && !isBoostActive()) {
        targetTemp = getScheduledTarget();
    }

    // 2. GESTIONE BOOST E OVERRIDE
    if (isBoostActive()) {
        targetTemp = TARGET_HEAT_ON; // O una temp specifica di boost se preferisci
    }
    else if (_manualOverride) {
        targetTemp = TARGET_HEAT_OFF; // Forza spento/eco
    }

    // 3. ISTERESI
    if (this->currentTemp < (targetTemp - TEMP_HYSTERESIS)) {
        if (!isHeating) startHeating();
    } 
    else if (this->currentTemp > (targetTemp + TEMP_HYSTERESIS)) {
        if (isHeating) stopHeating();
    }
}

void Thermostat::update(float temp) {
    // Usato solo per debug o sensori esterni push
    this->currentTemp = temp;
}

void Thermostat::setTarget(float target) { 
    this->targetTemp = target; 
    // Nota: in modalità AUTO questo viene sovrascritto al prossimo run()
    // A meno che non implementi una logica "Manuale Temporaneo" diversa dall'override OFF.
}

float Thermostat::getTarget() { return targetTemp; }