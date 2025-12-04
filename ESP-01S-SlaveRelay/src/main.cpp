#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include "secrets.h"

// GPIO0 è il pin del relè su ESP-01S Relay v1.0
#define RELAY_PIN 0 

ESP8266WebServer server(80);

void handleOn() {
  digitalWrite(RELAY_PIN, LOW); // Attivo LOW
  server.send(200, "application/json", "{\"status\":\"ON\"}");
  Serial.println("Comando: ON");
}

void handleOff() {
  digitalWrite(RELAY_PIN, HIGH); // Disattivo HIGH
  server.send(200, "application/json", "{\"status\":\"OFF\"}");
  Serial.println("Comando: OFF");
}

// NUOVO: Endpoint per Heartbeat
void handleStatus() {
  int state = digitalRead(RELAY_PIN);
  String s = (state == LOW) ? "ON" : "OFF";
  server.send(200, "application/json", "{\"status\":\"" + s + "\"}");
  Serial.println("Heartbeat Ping. Stato: " + s);
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Parte spento

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  
  if (!wm.autoConnect("Relay_Setup")) {
    Serial.println("Connessione fallita, riavvio...");
    ESP.restart();
  }

  // Imposta IP Statico (Verifica che corrisponda a CONFIG.H del Master)
  IPAddress local_IP(192, 168, 1, 33);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(local_IP, gateway, subnet);

  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/status", handleStatus); // Registra endpoint stato
  
  server.begin();
  Serial.println("Server Relè Avviato");
}

void loop() {
  server.handleClient();
}