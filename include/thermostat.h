#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include <Arduino.h>

class Thermostat {
public:
    Thermostat();
    void update(float currentTemp);
    void setTarget(float target);
    float getTarget();
    float getCurrentTemp();
    bool isHeatingState();
    float readLocalSensor();
    void startHeating();
    void stopHeating();

private:
    float currentTemp = 0.0;
    float targetTemp = 20.0;
    bool isHeating = false;
};

#endif