#ifndef INFLUX_MANAGER_H
#define INFLUX_MANAGER_H

#include <Arduino.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

class InfluxManager {
public:
    void begin();
    void loop(); // Necessario per evitare undefined reference
    
    // Metodi per inviare le categorie della tua tabella
    void reportSystemMetrics();
    void reportSensorMetrics(float temp, float hum, float target);
    void reportRelayState(bool state, const char* source);
    void reportNetworkMetrics();
    
    // Metodo generico per eventi (UI, Sicurezza, ecc.)
    void reportEvent(const char* category, const char* action, const char* details);
};

extern InfluxManager influx;

#endif