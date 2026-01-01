#include "influx_manager.h"
#include "secrets.h"
#include "config.h"
#include "config_manager.h" 
#include <WiFi.h>

// Puntatore globale per l'allocazione dinamica
InfluxDBClient* influxClient = nullptr;

InfluxManager influx;

void writePointDebug(Point& p) {
    if (influxClient == nullptr) return; 
    if (!influxClient->writePoint(p)) {
        Serial.print("InfluxDB Write Failed: ");
        Serial.println(influxClient->getLastErrorMessage());
    }
}

void InfluxManager::begin() {
    timeSync(configManager.data.timezone, "pool.ntp.org", "time.nist.gov");

    if (influxClient != nullptr) {
        delete influxClient;
        influxClient = nullptr;
    }

    if (strlen(configManager.data.influxUrl) == 0 || strlen(configManager.data.influxToken) == 0) {
        return;
    }

    influxClient = new InfluxDBClient(
        configManager.data.influxUrl,
        configManager.data.influxOrg,
        configManager.data.influxBucket,
        configManager.data.influxToken
    );

    influxClient->setInsecure(); 

    if (influxClient->validateConnection()) {
        Serial.print("Connesso a InfluxDB: ");
        Serial.println(influxClient->getServerUrl());
    } else {
        Serial.print("Errore InfluxDB: ");
        Serial.println(influxClient->getLastErrorMessage());
    }
}

void InfluxManager::loop() {}

void InfluxManager::reportSystemMetrics() {
    if(!WiFi.isConnected()) return;
    Point p("system");
    p.addTag("device", DEVICE_NAME);
    p.addField("heap_free", (long)ESP.getFreeHeap());
    p.addField("uptime_sec", (long)(millis() / 1000));
    p.addField("wifi_rssi", (long)WiFi.RSSI());
    writePointDebug(p);
}

// AGGIORNATO: Include Pressure
void InfluxManager::reportSensorMetrics(float temp, float hum, float pressure, float target) {
    if(!WiFi.isConnected()) return;

    Point p("sensors");
    p.addTag("device", DEVICE_NAME);

    if (!isnan(temp)) p.addField("temperature", temp);
    if (!isnan(hum)) p.addField("humidity", hum);
    // Nuovo campo Pressure
    if (!isnan(pressure) && pressure > 0) p.addField("pressure", pressure);
    
    p.addField("target_temp", target);
    
    writePointDebug(p);
}

void InfluxManager::reportRelayState(bool state, const char* source) {
    if(!WiFi.isConnected()) return;
    Point p("relay");
    p.addTag("device", DEVICE_NAME);
    p.addTag("source", source); 
    p.addField("state", state ? 1 : 0);
    writePointDebug(p);
}

void InfluxManager::reportNetworkMetrics() {
    if(!WiFi.isConnected()) return;
    Point p("network");
    p.addTag("device", DEVICE_NAME);
    p.addField("ip", WiFi.localIP().toString());
    p.addField("rssi", WiFi.RSSI());
    writePointDebug(p);
}

void InfluxManager::reportEvent(const char* category, const char* action, const char* details) {
    if(!WiFi.isConnected()) return;
    Point p("events");
    p.addTag("device", DEVICE_NAME);
    p.addTag("category", category);
    p.addTag("action", action);
    p.addField("details", details);
    writePointDebug(p);
}