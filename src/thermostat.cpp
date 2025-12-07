#include "thermostat.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"
#include "config_manager.h"
#include <time.h>

#define SENSOR_READ_INTERVAL 5000 
#define RELAY_TIMEOUT_MS 15000 
#define HEARTBEAT_INTERVAL (60 * 1000)
#define DHT_PIN 17      // <--- DEFINISCI IL PIN
#define DHT_TYPE DHT11  // <--- DEFINISCI IL TIPO (DHT11 o DHT22)

Thermostat::Thermostat() {
    #ifdef RELAY_PIN
    if (RELAY_PIN >= 0) {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW);
    }
    #endif
    // IP di default finché non viene scoperto
    _relayIP.fromString(DEFAULT_REMOTE_RELAY_IP);

    // Inizializza sensore
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

// Implementazione checkHeartbeat con parametro force
void Thermostat::checkHeartbeat(bool force) {
    unsigned long now = millis();
    // Se forzato o scaduto timer, prova anche un ping HTTP per certezza
    if (force || (now - _lastHeartbeatTime > HEARTBEAT_INTERVAL)) {
        pingRelay();
        _lastHeartbeatTime = now;
    }
}

bool Thermostat::pingRelay() {
    if (WiFi.status() != WL_CONNECTED) return false;
    
    // Usa l'IP scoperto o quello di default
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
        _lastPacketTime = millis(); // Resetta anche il timeout UDP se risponde via HTTP
        Serial.println("Heartbeat HTTP: OK");
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
    if (now - _lastSensorRead > SENSOR_READ_INTERVAL) {
        float t = readLocalSensor();
        update(t); 
        _lastSensorRead = now;
    }
    
    // Heartbeat periodico
    checkHeartbeat(false);
}

bool Thermostat::isRelayOnline() { return _relayOnline; }
String Thermostat::getRelayIP() { return _relayIP.toString(); }

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
    return false;
}

bool Thermostat::stopBoost() {
    _boostActive = false;
    _boostEndTime = 0;
    update(currentTemp); 
    return isRelayOnline();
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
    // Leggi temperatura e umidità 
    float t = _dht->readTemperature();
    float h = _dht->readHumidity();

    // Check se la lettura è fallita (NaN)
    if (isnan(t) || isnan(t)) {
        Serial.println("Errore lettura DHT!");
        // Ritorna l'ultimo valore valido o un valore di errore, 
        // per ora manteniamo la logica 'safe' ritornando la vecchia currentTemp
        return this->currentTemp; 
    }
    this->currentHumidity = h;

    // Correzione offset (opzionale, il display scalda!)
    // t = t - 2.0; 
    
    Serial.printf("DHT -> Temp: %.1f°C | Hum: %.1f%%\n", t, h);
    return t;
}
// Implementazione del getter
float Thermostat::getHumidity() {
    return this->currentHumidity;
    }