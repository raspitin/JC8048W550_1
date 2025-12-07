#include "mqtt_manager.h"
#include "config.h"
#include "thermostat.h"
#include <WiFi.h>
#include <PubSubClient.h>

extern Thermostat thermo;
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastReconnectAttempt = 0;

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.printf("MQTT Rx [%s]: %s\n", topic, message.c_str());

    // Cambio Target Temperatura da remoto
    if (String(topic) == MQTT_TOPIC_SET) {
        float newTarget = message.toFloat();
        if (newTarget > 10 && newTarget < 35) {
            thermo.setTarget(newTarget);
        }
    }
    // Accensione/Spegnimento da remoto
    else if (String(topic) == MQTT_TOPIC_MODE) {
        if (message == "heat") thermo.startHeating();
        else if (message == "off") thermo.stopHeating();
    }
}

boolean reconnect() {
    if (client.connect("ESP32Thermostat", MQTT_USER, MQTT_PASS)) {
        Serial.println("MQTT Connesso");
        // Sottoscrizione ai comandi
        client.subscribe(MQTT_TOPIC_SET);
        client.subscribe(MQTT_TOPIC_MODE);
    }
    return client.connected();
}

void mqtt_setup() {
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
}

void mqtt_loop() {
    if (!client.connected()) {
        long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        client.loop();
    }
}

void mqtt_publish_state(float currentTemp, float targetTemp, bool isHeating) {
    if (!client.connected()) return;
    
    client.publish(MQTT_TOPIC_TEMP, String(currentTemp, 1).c_str());
    // Pubblica Umidità (Aggiungi #define MQTT_TOPIC_HUM in config.h)
    client.publish("home/thermostat/humidity", String(thermo.getHumidity(), 0).c_str()); 
    
    client.publish(MQTT_TOPIC_TARGET, String(targetTemp, 1).c_str());
    client.publish(MQTT_TOPIC_STATE, isHeating ? "heating" : "off");
}