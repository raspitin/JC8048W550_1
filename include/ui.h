#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <lvgl.h>

// Variabili globali UI visibili agli altri file
extern lv_obj_t *scr_main;
extern lv_obj_t *scr_program;
extern lv_obj_t *scr_setup;
extern lv_obj_t *scr_impegni;

// Inizializza tutte le schermate
void ui_init_all();

// Splash Screen
void ui_show_splash();
void ui_splash_config_mode();

// Aggiorna i dati dinamici
void update_ui();

// Aggiorna meteo
void update_current_weather(String temp, String desc, String iconCode);
void update_forecast_item(int index, String day, String temp, String desc, String iconCode);

// Mostra popup errore relè
void show_relay_error_popup();

#endif