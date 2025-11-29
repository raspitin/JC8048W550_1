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

// Variabili cache per il meteo
String ui_temp = "--";
String ui_desc = "In attesa...";

void create_main_ui() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); // Sfondo nero

    // ORA
    lbl_time = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_time, LV_ALIGN_CENTER, 0, -60);
    lv_label_set_text(lbl_time, "--:--");

    // DATA
    lbl_date = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_date, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(lbl_date, LV_ALIGN_CENTER, 0, -20);
    lv_label_set_text(lbl_date, "Avvio sistema...");

    // TEMP
    lbl_weather_temp = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_weather_temp, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(lbl_weather_temp, lv_color_hex(0xF1C40F), 0); // Oro
    lv_obj_align(lbl_weather_temp, LV_ALIGN_CENTER, 0, 30);
    lv_label_set_text(lbl_weather_temp, "--");

    // DESCRIZIONE
    lbl_weather_desc = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_weather_desc, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_weather_desc, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_weather_desc, LV_ALIGN_CENTER, 0, 70);
    lv_label_set_text(lbl_weather_desc, "Caricamento meteo...");
    
    // IP
    lbl_ip = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_ip, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_ip, lv_color_hex(0x00FF00), 0);
    lv_obj_align(lbl_ip, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_label_set_text(lbl_ip, "Init...");
}

void update_weather_ui(String temp, String desc) {
    ui_temp = temp;
    ui_desc = desc;
    if(lbl_weather_temp) lv_label_set_text_fmt(lbl_weather_temp, "%s °C", ui_temp.c_str());
    if(lbl_weather_desc) lv_label_set_text(lbl_weather_desc, ui_desc.c_str());
}

void update_ui() {
    // 1. Gestione Ora
    struct tm timeinfo;
    if(getLocalTime(&timeinfo, 10)){
        char timeBuf[10];
        strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);
        lv_label_set_text(lbl_time, timeBuf);

        char dateBuf[32];
        strftime(dateBuf, sizeof(dateBuf), "%a %d %b %Y", &timeinfo);
        lv_label_set_text(lbl_date, dateBuf);
    }

    // 2. Gestione IP
    if (WiFi.status() == WL_CONNECTED) {
         String ip = WiFi.localIP().toString();
         lv_label_set_text_fmt(lbl_ip, "IP: %s", ip.c_str());
    } else {
         lv_label_set_text(lbl_ip, "WiFi Disconnesso");
    }
}