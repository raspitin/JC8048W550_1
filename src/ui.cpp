#include <lvgl.h>
#include "ui.h"
#include <WiFi.h>
#include <time.h>

// Widget globali
lv_obj_t *lbl_time;
lv_obj_t *lbl_date;
lv_obj_t *lbl_weather_temp;
lv_obj_t *lbl_weather_desc;
lv_obj_t *lbl_ip;

// Variabili dati meteo (cache)
String ui_temp = "--";
String ui_desc = "In attesa dati...";

void create_main_ui() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); // Sfondo Nero

    // --- ORA (Grande, al centro in alto) ---
    lbl_time = lv_label_create(scr);
    // Usa il font più grande disponibile (36 o 48 se abilitato in lv_conf)
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_time, LV_ALIGN_CENTER, 0, -60);
    lv_label_set_text(lbl_time, "00:00");

    // --- DATA (Sotto l'ora) ---
    lbl_date = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_date, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(lbl_date, LV_ALIGN_CENTER, 0, -20);
    lv_label_set_text(lbl_date, "---");

    // --- TEMPERATURA METEO (Giallo/Oro) ---
    lbl_weather_temp = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_weather_temp, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(lbl_weather_temp, lv_color_hex(0xF1C40F), 0);
    lv_obj_align(lbl_weather_temp, LV_ALIGN_CENTER, 0, 30);
    lv_label_set_text(lbl_weather_temp, "-- °C");

    // --- DESCRIZIONE METEO ---
    lbl_weather_desc = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_weather_desc, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_weather_desc, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_weather_desc, LV_ALIGN_CENTER, 0, 70);
    lv_label_set_text(lbl_weather_desc, "Caricamento...");
    
    // --- IP Address (Piccolo in basso per debug) ---
    lbl_ip = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_ip, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_ip, lv_color_hex(0x00FF00), 0);
    lv_obj_align(lbl_ip, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_label_set_text(lbl_ip, "WiFi...");
}

void update_weather_ui(String temp, String desc) {
    ui_temp = temp;
    ui_desc = desc;
    // Forza aggiornamento immediato etichette se sono state create
    if(lbl_weather_temp) lv_label_set_text_fmt(lbl_weather_temp, "%s °C", ui_temp.c_str());
    if(lbl_weather_desc) lv_label_set_text(lbl_weather_desc, ui_desc.c_str());
}

void update_ui() {
    // Aggiorna Orologio e Data dal sistema
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)){
        char timeBuf[10];
        strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);
        lv_label_set_text(lbl_time, timeBuf);

        char dateBuf[32];
        // Formato: Dom 23 Nov 2025
        strftime(dateBuf, sizeof(dateBuf), "%a %d %b %Y", &timeinfo);
        lv_label_set_text(lbl_date, dateBuf);
    }

    // Aggiorna IP se connesso
    if (WiFi.status() == WL_CONNECTED) {
         String ip = WiFi.localIP().toString();
         lv_label_set_text_fmt(lbl_ip, "IP: %s", ip.c_str());
    } else {
         lv_label_set_text(lbl_ip, "WiFi Disconnesso");
    }
}