#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "web_interface.h"
#include "thermostat.h"
#include "network_manager.h"

AsyncWebServer server(80);
extern Thermostat thermo;

void setup_web_server() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Termostato Online. API: /status, /set?target=22");
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"current\":" + String(thermo.getCurrentTemp()) + ",";
        json += "\"target\":" + String(thermo.getTarget()) + ",";
        json += "\"heating\":" + String(thermo.isHeatingState() ? "true" : "false");
        json += "}";
        request->send(200, "application/json", json);
    });

    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("target")) {
            thermo.setTarget(request->getParam("target")->value().toFloat());
        }
        request->send(200, "text/plain", "OK");
    });
    
    server.on("/reset_wifi", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Resetting WiFi...");
        wifi_reset_settings();
    });

    server.begin();
}