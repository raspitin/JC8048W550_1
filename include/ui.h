#ifndef UI_H
#define UI_H
#include <Arduino.h>

// Inizializza tutte le schermate (Home, Caldaia, Setup, Impegni)
void ui_init_all();

// Aggiorna i dati dinamici (Orario, WiFi, ecc.)
void update_ui();

// Aggiorna meteo corrente
void update_current_weather(String temp, String desc, String iconCode);

// Aggiorna un box previsione
void update_forecast_item(int index, String day, String temp, String iconCode);

#endif