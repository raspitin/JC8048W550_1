#ifndef UI_H
#define UI_H

#include <Arduino.h>

// Inizializza tutte le schermate
void ui_init_all();

// Aggiorna i dati dinamici
void update_ui();

// Aggiorna meteo
void update_current_weather(String temp, String desc, String iconCode);
void update_forecast_item(int index, String day, String temp, String iconCode);

// NUOVO: Mostra popup errore relè
void show_relay_error_popup();

#endif