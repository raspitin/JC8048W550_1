#ifndef UI_H
#define UI_H
#include <Arduino.h>

// Inizializza l'interfaccia grafica principale (Data, Ora, Meteo)
void create_main_ui();

// Aggiorna i dati a schermo (da chiamare nel loop)
void update_ui();

// Passa i dati meteo alla UI
void update_weather_ui(String temp, String desc);

#endif