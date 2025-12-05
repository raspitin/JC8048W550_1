#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Inizializza il server web
void setup_web_server();

// Handler Semplici
void handleRoot(AsyncWebServerRequest *request);
void handleGetStatus(AsyncWebServerRequest *request);
void handleSetBoost(AsyncWebServerRequest *request);
void handleGetSchedule(AsyncWebServerRequest *request);
void handleGetImpegni(AsyncWebServerRequest *request);

// Handler Complessi (Body Handlers per POST JSON)
// Devono avere questa firma esatta per essere accettati da server.on()
void handleSetSchedule(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
void handleSetImpegni(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

#endif