#ifndef _UI_H
#define _UI_H
#include "lvgl.h"
#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif



// --- SCHERMATE ---
extern lv_obj_t *scr_splash;
extern lv_obj_t *scr_main;
extern lv_obj_t *scr_program;
extern lv_obj_t *scr_setup;
extern lv_obj_t *scr_impegni;

// --- WIDGET GLOBALI ---
extern lv_obj_t *ui_lbl_hour;
extern lv_obj_t *ui_lbl_min;
extern lv_obj_t *ui_lbl_dots;

// Widget Home - Temperatura & Umidità
extern lv_obj_t *ui_lbl_temp_val;
extern lv_obj_t *ui_lbl_hum_val;

// --- NUOVI WIDGET PRESSIONE ---
extern lv_obj_t *ui_lbl_press_icon; // Icona (o etichetta "hPa")
extern lv_obj_t *ui_lbl_press_val;  // Valore numerico

// --- FUNZIONI ---
void ui_init_all();
void ui_show_splash();
void ui_create_main_screen();
void update_ui(); 
void update_current_weather(String temp, String desc, String iconCode);
void update_forecast_item(int index, String day, String temp, String desc, String iconCode);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif