#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 

// GPIO0 è il pin del relè su ESP-01S Relay v1.0
#define RELAY_PIN 0 

ESP8266WebServer server(80);

void handleOn() {
  digitalWrite(RELAY_PIN, LOW); // Attivo LOW per questo modulo
  server.send(200, "application/json", "{\"status\":\"ON\"}");
  Serial.println("Comando ricevuto: RELAY ON");
}

void handleOff() {
  digitalWrite(RELAY_PIN, HIGH); // Disattivo HIGH
  server.send(200, "application/json", "{\"status\":\"OFF\"}");
  Serial.println("Comando ricevuto: RELAY OFF");
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Parte spento

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  
  // Crea un AP chiamato "Relay_Setup" se non trova il WiFi
  if (!wm.autoConnect("Relay_Setup")) {
    Serial.println("Connessione fallita, riavvio...");
    ESP.restart();
  }

  // Imposta IP Statico (FONDAMENTALE affinché il Master lo trovi)
  // Modifica questi valori in base alla tua rete!
  IPAddress local_IP(192, 168, 1, 200);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  // Applica configurazione IP statico
  WiFi.config(local_IP, gateway, subnet);

  server.on("/on", handleOn);
  server.on("/off", handleOff);
  
  server.begin();
  Serial.println("Server Relè Avviato");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
}
