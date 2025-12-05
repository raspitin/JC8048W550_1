#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include <Arduino.h>
#include <time.h>

class Thermostat {
public:
    Thermostat();
    
    // Metodo principale da chiamare nel loop()
    void run(); 

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

private:
    float currentTemp = 0.0;
    float targetTemp = 20.0;
    bool isHeating = false;
    
    bool _boostActive = false;
    time_t _boostEndTime = 0;

    unsigned long _lastHeartbeatTime = 0;
    unsigned long _lastSensorRead = 0; 
    bool _relayOnline = true; 

    bool sendRelayCommand(bool turnOn);
    bool pingRelay(); 
};

#endif