#ifndef UI_H
#define UI_H
#include <Arduino.h>

// Inizializza tutte le schermate e la navigazione
void ui_init_all();

// Aggiorna i dati dinamici (chiama nel loop)
void update_ui();

// Aggiorna meteo corrente
void update_current_weather(String temp, String desc, String iconCode);

// Aggiorna previsioni
void update_forecast_item(int index, String day, String temp, String iconCode);

#endif