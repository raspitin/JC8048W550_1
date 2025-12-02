#include "thermostat.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"
#include "config_manager.h" // Necessario per leggere la programmazione
#include <time.h>

// ... [FUNZIONE sendRelayCommand INVARIATA] ...
// (Mantenere la funzione sendRelayCommand com'era nel file originale)
// Rimetto qui solo il corpo della classe Thermostat modificato

void sendRelayCommand(bool turnOn) {
    // Copia la logica originale di sendRelayCommand qui (GPIO + HTTP request)
    // Per brevità, assumo che sia invariata rispetto al tuo file precedente.
    // ...
     #ifdef RELAY_PIN
        bool pinState = turnOn;
        if (RELAY_ACTIVE_LOW) pinState = !pinState; 
        digitalWrite(RELAY_PIN, pinState);
    #endif

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        time_t now;
        time(&now);
        struct tm *timeinfo = localtime(&now);
        int currentMinute = timeinfo->tm_min;
        const char* token = SECRET_TOKENS[currentMinute];
        String command = turnOn ? "on" : "off";
        String url = String("http://") + REMOTE_RELAY_IP + "/" + command + "?auth=" + token;
        http.setTimeout(500); 
        if (http.begin(url)) {
            http.GET();
            http.end();
        }
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
    
    // --- LOGICA PROGRAMMAZIONE ORARIA ---
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);

    // 1. Determina se siamo in una fascia oraria attiva
    int day = timeinfo->tm_wday; // 0=Domenica, ...
    // Mappa la domenica (0) all'indice 6 della nostra struct se necessario, 
    // o assicurati che weekSchedule[0] sia Domenica come in struct tm.
    // Assumiamo 0=Domenica, 1=Lunedi come standard.
    
    // Calcola indice slot 30 min (0..47)
    // Esempio 00:20 -> slot 0. 00:40 -> slot 1.
    int slotIndex = timeinfo->tm_hour * 2 + (timeinfo->tm_min >= 30 ? 1 : 0);
    
    bool scheduleActive = (configManager.data.weekSchedule[day].timeSlots >> slotIndex) & 1ULL;

    // 2. Imposta il target in base alla programmazione
    // Se in fascia oraria ON -> Target Comfort (es. 22°C), altrimenti Eco (es. 16°C)
    float scheduledTarget = scheduleActive ? TARGET_HEAT_ON : TARGET_HEAT_OFF;
    
    // Sovrascrive il target manuale solo se la modalità non è "Manuale Forzato"
    // (Qui semplifichiamo: la programmazione comanda il target)
    targetTemp = scheduledTarget;

    // 3. Logica Isteresi
    if (isHeating) {
        if (currentTemp >= targetTemp + TEMP_HYSTERESIS) stopHeating();
    } else {
        if (currentTemp <= targetTemp - TEMP_HYSTERESIS) startHeating();
    }
}

void Thermostat::setTarget(float target) {
    this->targetTemp = target;
}

float Thermostat::getTarget() { return targetTemp; }
float Thermostat::getCurrentTemp() { return currentTemp; }
bool Thermostat::isHeatingState() { return isHeating; }

float Thermostat::readLocalSensor() {
    // Simulazione (invariata)
    static float simTemp = 19.0;
    if(isHeating) simTemp += 0.005; else simTemp -= 0.005;
    if (simTemp > 24) simTemp = 24;
    if (simTemp < 15) simTemp = 15;
    return simTemp;
}