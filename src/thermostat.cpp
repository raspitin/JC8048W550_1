#include "thermostat.h"
#include <Arduino.h>

// Placeholder: In un sistema reale, qui controlleresti il relè o chiameresti un ESP slave
#define RELAY_PIN 4 

Thermostat::Thermostat() {
    // pinMode(RELAY_PIN, OUTPUT); 
}

void Thermostat::startHeating() {
    isHeating = true;
    // digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Heating ON");
}

void Thermostat::stopHeating() {
    isHeating = false;
    // digitalWrite(RELAY_PIN, LOW);
    Serial.println("Heating OFF");
}

void Thermostat::update(float currentTemp) {
    this->currentTemp = currentTemp;
    if (isHeating) {
        if (currentTemp >= targetTemp + 0.5) stopHeating();
    } else {
        if (currentTemp <= targetTemp - 0.5) startHeating();
    }
}

void Thermostat::setTarget(float target) {
    this->targetTemp = target;
    update(this->currentTemp); 
}

float Thermostat::getTarget() { return targetTemp; }
float Thermostat::getCurrentTemp() { return currentTemp; }
bool Thermostat::isHeatingState() { return isHeating; }

float Thermostat::readLocalSensor() {
    // Simulazione sensore
    static float simTemp = 19.0;
    if(isHeating) simTemp += 0.01; else simTemp -= 0.01;
    if (simTemp > 30) simTemp = 30;
    if (simTemp < 10) simTemp = 10;
    return simTemp;
}