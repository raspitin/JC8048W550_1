#ifndef UI_H
#define UI_H
#include <Arduino.h>

// Inizializza l'interfaccia grafica principale
void create_main_ui();

// Aggiorna orologio e stato WiFi (loop veloce)
void update_ui();

// Aggiorna i dati meteo attuali
void update_current_weather(String temp, String desc, String iconCode);

// Aggiorna un singolo box della previsione (indice 0-4)
// day: "Lun", temp: "22", iconCode: codice meteo OWM (es "01d")
void update_forecast_item(int index, String day, String temp, String iconCode);

#endif