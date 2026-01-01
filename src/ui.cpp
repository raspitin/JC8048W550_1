#include <lvgl.h>
#include "ui.h"
#include "network_manager.h" 
#include "config_manager.h"
#include <WiFi.h>
#include <time.h>
#include "config.h" 
#include <LittleFS.h>     
#include <ArduinoJson.h>  

#include "thermostat.h"
extern Thermostat thermo;

// DICHIARAZIONE IMMAGINI
LV_IMG_DECLARE(logo_splash);
// LV_IMG_DECLARE(qr_code); 

// --- OGGETTI UI ---
lv_obj_t *scr_splash = NULL;
lv_obj_t *scr_main = NULL;

// Widget Splash
lv_obj_t *lbl_splash_status = NULL;

// Widget Home
lv_obj_t *ui_lbl_hour = NULL;
lv_obj_t *ui_lbl_min = NULL;
lv_obj_t *ui_lbl_dots = NULL;

lv_obj_t *ui_lbl_temp_val = NULL;
lv_obj_t *ui_lbl_hum_val = NULL;

// --- NUOVI OGGETTI PRESSIONE ---
lv_obj_t *ui_lbl_press_icon = NULL;
lv_obj_t *ui_lbl_press_val = NULL;

// Stili base
static lv_style_t style_text_large;
static lv_style_t style_text_small;

void ui_init_style() {
    lv_style_init(&style_text_large);
    lv_style_set_text_font(&style_text_large, &lv_font_montserrat_36); 
    lv_style_set_text_color(&style_text_large, lv_color_white());

    lv_style_init(&style_text_small);
    lv_style_set_text_font(&style_text_small, &lv_font_montserrat_16); 
    lv_style_set_text_color(&style_text_small, lv_color_white());
}

// Mostra lo Splash Screen con logo e label di stato
void ui_show_splash() {
    if (scr_splash) return;
    scr_splash = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_splash, lv_color_hex(0x000000), 0);
    
    lv_obj_t *img = lv_img_create(scr_splash);
    lv_img_set_src(img, &logo_splash);
    lv_obj_center(img);
    
    // Creazione Label Stato
    lbl_splash_status = lv_label_create(scr_splash);
    lv_obj_add_style(lbl_splash_status, &style_text_small, 0);
    lv_label_set_text(lbl_splash_status, "Avvio...");
    lv_obj_align(lbl_splash_status, LV_ALIGN_BOTTOM_MID, 0, -30);
    
    lv_scr_load(scr_splash);
}

// Chiamata dal Network Manager quando entra in AP Mode
void ui_splash_config_mode() {
    if (lbl_splash_status) {
        lv_label_set_text(lbl_splash_status, "AP Mode: Connettiti al WiFi");
    }
}

void ui_create_main_screen() {
    if (scr_main) return;
    scr_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_main, lv_color_hex(0x202020), 0);

    // --- TEMPERATURA GRANDE (Centro) ---
    ui_lbl_temp_val = lv_label_create(scr_main);
    lv_obj_add_style(ui_lbl_temp_val, &style_text_large, 0);
    lv_label_set_text(ui_lbl_temp_val, "--.-°C");
    lv_obj_align(ui_lbl_temp_val, LV_ALIGN_CENTER, 0, -20);

    // --- UMIDITA' (Sotto Temp) ---
    ui_lbl_hum_val = lv_label_create(scr_main);
    lv_obj_add_style(ui_lbl_hum_val, &style_text_small, 0);
    lv_label_set_text(ui_lbl_hum_val, "--%");
    lv_obj_align_to(ui_lbl_hum_val, ui_lbl_temp_val, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // --- NUOVO: PRESSIONE (Sotto Umidità) ---
    ui_lbl_press_val = lv_label_create(scr_main);
    lv_obj_add_style(ui_lbl_press_val, &style_text_small, 0);
    lv_label_set_text(ui_lbl_press_val, "-- hPa");
    lv_obj_align_to(ui_lbl_press_val, ui_lbl_hum_val, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // --- OROLOGIO (In alto a destra) ---
    ui_lbl_hour = lv_label_create(scr_main);
    lv_obj_add_style(ui_lbl_hour, &style_text_large, 0);
    lv_label_set_text(ui_lbl_hour, "00");
    lv_obj_align(ui_lbl_hour, LV_ALIGN_TOP_RIGHT, -60, 10);

    ui_lbl_dots = lv_label_create(scr_main);
    lv_obj_add_style(ui_lbl_dots, &style_text_large, 0);
    lv_label_set_text(ui_lbl_dots, ":");
    lv_obj_align_to(ui_lbl_dots, ui_lbl_hour, LV_ALIGN_OUT_RIGHT_MID, 2, 0);

    ui_lbl_min = lv_label_create(scr_main);
    lv_obj_add_style(ui_lbl_min, &style_text_large, 0);
    lv_label_set_text(ui_lbl_min, "00");
    lv_obj_align_to(ui_lbl_min, ui_lbl_dots, LV_ALIGN_OUT_RIGHT_MID, 2, 0);
}

void update_ui() {
    // Aggiornamento Orologio
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    
    if (ui_lbl_hour && ui_lbl_min) {
        char buf[4];
        sprintf(buf, "%02d", timeinfo->tm_hour);
        lv_label_set_text(ui_lbl_hour, buf);
        sprintf(buf, "%02d", timeinfo->tm_min);
        lv_label_set_text(ui_lbl_min, buf);
        
        // Lampeggio due punti
        if (timeinfo->tm_sec % 2 == 0) lv_obj_clear_flag(ui_lbl_dots, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(ui_lbl_dots, LV_OBJ_FLAG_HIDDEN);
    }

    // Aggiornamento Sensori
    if (ui_lbl_temp_val) {
        float t = thermo.getCurrentTemp();
        float h = thermo.getHumidity();
        float p = thermo.getPressure(); 

        if (isnan(t)) lv_label_set_text(ui_lbl_temp_val, "--.-°C");
        else lv_label_set_text_fmt(ui_lbl_temp_val, "%.1f°C", t);

        if (isnan(h)) lv_label_set_text(ui_lbl_hum_val, "--%");
        else lv_label_set_text_fmt(ui_lbl_hum_val, "%.0f%%", h);

        if (ui_lbl_press_val) {
            if (isnan(p) || p <= 0) lv_label_set_text(ui_lbl_press_val, "-- hPa");
            else lv_label_set_text_fmt(ui_lbl_press_val, "%.0f hPa", p);
        }
    }
}

void ui_init_all() {
    ui_init_style();
    ui_create_main_screen();
}

void update_current_weather(String temp, String desc, String iconCode) {
    // Implementazione vuota per ora, da riempire se hai le icone meteo
}
void update_forecast_item(int index, String day, String temp, String desc, String iconCode) {
    // Implementazione vuota per ora
}