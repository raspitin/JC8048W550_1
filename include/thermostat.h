#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include <Arduino.h>
#include <WiFiUdp.h> 
#include <time.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>

class Thermostat {
public:
    Thermostat();
    void setup(); 
    void run();   

    // Metodi principali
    void update(float currentTemp); // Usato per test o override manuale valore
    void setTarget(float target);
    float getTarget();
    
    // Getter Sensori
    float getCurrentTemp();
    float getHumidity();
    float getPressure(); // Nuovo
    
    bool isHeatingState();

    // Boost & Manuale
    bool startBoost(int minutes);
    bool stopBoost();
    bool isBoostActive();
    long getBoostRemainingSeconds();
    
    // Toggle Override
    void toggleOverride(); 
    bool isOverrideActive();

    // Manuale diretto
    bool startHeating();
    bool stopHeating();

    // Heartbeat & Stato Relè Remoto
    void checkHeartbeat(bool force = false);
    bool isRelayOnline(); 
    String getRelayIP();

private:
    // Sensori I2C
    Adafruit_AHTX0 aht;
    Adafruit_BMP280 bmp;
    bool sensorsReady = false;
    
    // Caching letture
    unsigned long last_read_time;
    float cached_temp;
    float cached_hum;
    float cached_press;
    void readSensors(); // Funzione interna di lettura

    // Stato Termostato
    float currentTemp = 0.0; // Valore usato per la logica (spesso == cached_temp)
    float currentHumidity = 0.0;
    float targetTemp = 19.0;
    bool isHeating = false;
    
    bool _boostActive = false;
    time_t _boostEndTime = 0;
    
    bool _manualOverride = false; 
    int _lastScheduleSlot = -1; 

    // Gestione Rete UDP
    WiFiUDP _discoveryUdp;
    IPAddress _relayIP;     
    bool _relayOnline = false;
    unsigned long _lastHeartbeat = 0;

    void checkDiscovery();
    void sendDiscovery();
    float getScheduledTarget();
};

#endif