#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include <Arduino.h>
#include <WiFiUdp.h> // Necessario per Auto-Discovery
#include <time.h>

class Thermostat {
public:
    Thermostat();
    void setup(); // Inizializza UDP
    void run();   // Loop principale

    void update(float currentTemp);
    void setTarget(float target);
    float getTarget();
    float getCurrentTemp();
    bool isHeatingState();
    float readLocalSensor();
    
    // Boost
    bool startBoost(int minutes);
    bool stopBoost();
    bool isBoostActive();
    long getBoostRemainingSeconds();

    // Manuale
    bool startHeating();
    bool stopHeating();

    // Heartbeat & Stato
    // Il parametro 'force' permette di controllare subito senza aspettare il timer
    void checkHeartbeat(bool force = false);
    bool isRelayOnline(); 
    String getRelayIP();

private:
    float currentTemp = 0.0;
    float targetTemp = 20.0;
    bool isHeating = false;
    
    bool _boostActive = false;
    time_t _boostEndTime = 0;

    // Gestione Rete
    WiFiUDP _discoveryUdp;
    IPAddress _relayIP;     // L'IP scoperto dinamicamente
    bool _relayOnline = false; // Default false finché non lo trova
    unsigned long _lastPacketTime = 0;
    unsigned long _lastSensorRead = 0; 
    unsigned long _lastHeartbeatTime = 0;

    bool sendRelayCommand(bool turnOn);
    void checkDiscovery(); 
    bool pingRelay();
};

#endif