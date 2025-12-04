#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include <Arduino.h>
#include <time.h>

class Thermostat {
public:
    Thermostat();
    void update(float currentTemp);
    void setTarget(float target);
    float getTarget();
    float getCurrentTemp();
    bool isHeatingState();
    float readLocalSensor();
    
    // Boost (ritorna bool per successo connessione)
    bool startBoost(int minutes);
    bool stopBoost();
    bool isBoostActive();
    long getBoostRemainingSeconds();

    // Controllo manuale (ritorna bool per successo connessione)
    bool startHeating();
    bool stopHeating();

    // Heartbeat
    void checkHeartbeat();

private:
    float currentTemp = 0.0;
    float targetTemp = 20.0;
    bool isHeating = false;
    
    bool _boostActive = false;
    time_t _boostEndTime = 0;

    unsigned long _lastHeartbeatTime = 0;
    bool _relayOnline = true; 

    bool sendRelayCommand(bool turnOn);
    bool pingRelay(); 
};

#endif