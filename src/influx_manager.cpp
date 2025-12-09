#include "influx_manager.h"
#include "secrets.h"
#include "config.h"
#include "config_manager.h" 
#include <WiFi.h>

// Inizializzazione Client InfluxDB 2.0
InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

InfluxManager influx;

void InfluxManager::begin() {
    // Sincronizzazione oraria precisa
    timeSync(configManager.data.timezone, "pool.ntp.org", "time.nist.gov");

    // Impostazioni connessione
    influxClient.setInsecure(); 

    // Opzionale: stampa i log della libreria per debug profondo
    // influxClient.setWriteOptions(WriteOptions().writePrecision(WritePrecision::S).batchSize(1).bufferSize(4));

    if (influxClient.validateConnection()) { 
        Serial.print("Connesso a InfluxDB: ");
        Serial.println(influxClient.getServerUrl()); 
    } else {
        Serial.print("Errore connessione InfluxDB: ");
        Serial.println(influxClient.getLastErrorMessage()); 
    }
}

void InfluxManager::loop() {
    // Gestione buffer se necessario, per ora vuoto ma richiesto dal .h
}

// Funzione helper per scrivere e loggare errori
void writePointWithDebug(Point& p) {
    if (!influxClient.writePoint(p)) {
        Serial.print("InfluxDB Write Failed: ");
        Serial.println(influxClient.getLastErrorMessage());
    }
}

void InfluxManager::reportSystemMetrics() {
    if(!WiFi.isConnected()) return;

    Point p("system");
    p.addTag("device", DEVICE_NAME);
    
    p.addField("heap_free", (long)ESP.getFreeHeap()); // Cast espliciti per sicurezza
    p.addField("heap_max_block", (long)ESP.getMaxAllocHeap());
    
    float frag = 0.0;
    if (ESP.getFreeHeap() > 0) {
        frag = 100.0 - ((float)ESP.getMaxAllocHeap() / (float)ESP.getFreeHeap() * 100.0);
    }
    p.addField("heap_frag_pct", frag);
    p.addField("uptime_sec", (long)(millis() / 1000));
    p.addField("wifi_rssi", (long)WiFi.RSSI());
    
    writePointWithDebug(p);
}

void InfluxManager::reportSensorMetrics(float temp, float hum, float target) {
    if(!WiFi.isConnected()) return;

    Point p("sensors");
    p.addTag("device", DEVICE_NAME);

    if (!isnan(temp)) p.addField("temperature", temp);
    if (!isnan(hum)) p.addField("humidity", hum);
    p.addField("target_temp", target);
    
    if (!isnan(temp) && !isnan(hum)) {
        double a = 17.27;
        double b = 237.7;
        double alpha = ((a * temp) / (b + temp)) + log(hum/100.0);
        double dew_point = (b * alpha) / (a - alpha);
        p.addField("dew_point", dew_point);
    }

    float heat_need = target - temp;
    if (isnan(heat_need) || heat_need < 0) heat_need = 0;
    p.addField("heating_need", heat_need);

    writePointWithDebug(p);
}

void InfluxManager::reportRelayState(bool state, const char* source) {
    if(!WiFi.isConnected()) return;
    
    Point p("relay");
    p.addTag("device", DEVICE_NAME);
    p.addTag("source", source); 
    p.addField("state", state ? 1 : 0);
    
    writePointWithDebug(p);
}

void InfluxManager::reportNetworkMetrics() {
    if(!WiFi.isConnected()) return;

    Point p("network");
    p.addTag("device", DEVICE_NAME);
    p.addField("ip", WiFi.localIP().toString());
    p.addField("rssi", WiFi.RSSI());
    
    writePointWithDebug(p);
}

void InfluxManager::reportEvent(const char* category, const char* action, const char* details) {
    if(!WiFi.isConnected()) return;

    Point p("events");
    p.addTag("device", DEVICE_NAME);
    p.addTag("category", category);
    p.addTag("action", action);
    p.addField("details", details);
    
    writePointWithDebug(p);
}