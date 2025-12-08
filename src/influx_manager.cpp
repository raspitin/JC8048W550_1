#include "influx_manager.h"
#include "secrets.h"
#include "config.h"
#include "config_manager.h" 
#include <WiFi.h>

// Inizializzazione Client InfluxDB 2.0
// RINOMINATO DA client A influxClient PER EVITARE CONFLITTI CON MQTT
InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);

InfluxManager influx;

void InfluxManager::begin() {
    // Imposta la sincronizzazione oraria precisa per i timestamp dei dati
    timeSync(configManager.data.timezone, "pool.ntp.org", "time.nist.gov");

    // Impostazioni connessione
    influxClient.setInsecure(); // <--- influxClient

    if (influxClient.validateConnection()) { // <--- influxClient
        Serial.print("Connesso a InfluxDB: ");
        Serial.println(influxClient.getServerUrl()); // <--- influxClient
    } else {
        Serial.print("Errore connessione InfluxDB: ");
        Serial.println(influxClient.getLastErrorMessage()); // <--- influxClient
    }
}

void InfluxManager::reportSystemMetrics() {
    if(!WiFi.isConnected()) return;

    Point p("system");
    p.addTag("device", DEVICE_NAME);
    
    p.addField("heap_free", ESP.getFreeHeap());
    p.addField("heap_max_block", ESP.getMaxAllocHeap());
    
    // Calcolo percentuale di frammentazione
    float frag = 0.0;
    if (ESP.getFreeHeap() > 0) {
        frag = 100.0 - ((float)ESP.getMaxAllocHeap() / (float)ESP.getFreeHeap() * 100.0);
    }
    p.addField("heap_frag_pct", frag);

    p.addField("cpu_temp", temperatureRead()); 
    p.addField("uptime_sec", millis() / 1000);
    p.addField("wifi_rssi", WiFi.RSSI());
    
    influxClient.writePoint(p); // <--- influxClient
}

void InfluxManager::reportSensorMetrics(float temp, float hum, float target) {
    if(!WiFi.isConnected()) return;

    Point p("sensors");
    p.addTag("device", DEVICE_NAME);

    // Evita di inviare NAN se il sensore non è pronto
    if (!isnan(temp)) p.addField("temperature", temp);
    if (!isnan(hum)) p.addField("humidity", hum);
    p.addField("target_temp", target);
    
    // Calcolo Dew Point (Formula di Magnus)
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

    influxClient.writePoint(p); // <--- influxClient
}

void InfluxManager::reportRelayState(bool state, const char* source) {
    if(!WiFi.isConnected()) return;
    
    Point p("relay");
    p.addTag("device", DEVICE_NAME);
    p.addTag("source", source); 
    p.addField("state", state ? 1 : 0);
    
    influxClient.writePoint(p); // <--- influxClient
}

void InfluxManager::reportEvent(const char* category, const char* action, const char* details) {
    if(!WiFi.isConnected()) return;

    Point p("events");
    p.addTag("device", DEVICE_NAME);
    p.addTag("category", category);
    p.addTag("action", action);
    p.addField("details", details);
    
    influxClient.writePoint(p); // <--- influxClient
}