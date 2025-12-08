#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include <Arduino.h>
#include <WiFiUdp.h> 
#include <time.h>
#include <DHT.h>

class Thermostat {
public:
    Thermostat();
    void setup(); 
    void run();   

    void update(float currentTemp);
    void setTarget(float target);
    float getTarget();
    float getCurrentTemp();
    float getHumidity();
    bool isHeatingState();
    float readLocalSensor();
    
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

    // Heartbeat & Stato
    void checkHeartbeat(bool force = false);
    bool isRelayOnline(); 
    String getRelayIP();

private:
    float currentTemp = 0.0;
    float currentHumidity = 0.0;
    float targetTemp = 19.0;
    bool isHeating = false;
    
    bool _boostActive = false;
    time_t _boostEndTime = 0;
    
    bool _manualOverride = false; 
    int _lastScheduleSlot = -1; 

    // Gestione Rete
    WiFiUDP _discoveryUdp;
    IPAddress _relayIP;     
    bool _relayOnline = false; 
    
    // TIMERS
    unsigned long _lastPacketTime = 0;
    unsigned long _lastSensorRead = 0; 
    unsigned long _lastControlTime = 0; // <--- NUOVO: Timer per logica lenta
    unsigned long _lastHeartbeatTime = 0;
    bool _firstRun = true;              // <--- NUOVO: Per forzare il primo controllo

    bool sendRelayCommand(bool turnOn);
    void checkDiscovery(); 
    bool pingRelay();
    DHT* _dht;
};

#endif