#include <lvgl.h>
#include "ui.h"
#include "network_manager.h" 
#include "config_manager.h"
#include <WiFi.h>
#include <time.h>
#include "config.h" 

#include "thermostat.h"
extern Thermostat thermo;

// --- SCHERMATE ---
lv_obj_t *scr_main;
lv_obj_t *scr_caldaia;
lv_obj_t *scr_setup;
lv_obj_t *scr_impegni;

// --- WIDGET HOME GLOBALI ---
lv_obj_t *ui_lbl_hour = NULL;
lv_obj_t *ui_lbl_min = NULL;
lv_obj_t *ui_lbl_dots = NULL;
lv_obj_t *lbl_date;

// VARIABILI DISABILITATE PER TEST
/*
lv_obj_t *lbl_cur_temp_big; 
lv_obj_t *lbl_cur_desc;
lv_obj_t *cont_cur_icon;    
lv_obj_t *forecast_days[5];
lv_obj_t *forecast_temps[5];
lv_obj_t *forecast_icon_containers[5];
lv_obj_t *lbl_setup_ssid;
lv_obj_t *lbl_setup_ip;
lv_obj_t *lbl_setup_gw;
lv_obj_t *btn_manual_toggle;
lv_obj_t *lbl_manual_toggle;
*/

// --- WIDGET PROGRAMMAZIONE ---
int edit_day_idx = 0;
int edit_slot_idx = 0;
lv_obj_t *lbl_slot_time[7][4]; 
lv_obj_t *lbl_slot_temp[7][4];
lv_obj_t *win_editor;
lv_obj_t *roller_hour;
lv_obj_t *roller_min;
lv_obj_t *lbl_edit_temp;
float temp_editing = 20.0;

// ============================================================================
//  PROTOTIPI
// ============================================================================
// (Alcuni prototipi lasciati per evitare errori di compilazione se richiamati altrove)
void create_forecast_box(lv_obj_t *parent, int index);
void create_home_button(lv_obj_t *parent);
void render_weather_icon(lv_obj_t *parent, String code);

// ============================================================================
//  NAVIGAZIONE E EVENTI
// ============================================================================

static void nav_event_cb(lv_event_t * e) {
    // Disabilitato
}

// ============================================================================
//  BUILDERS PAGINE (SEMPLIFICATO)
// ============================================================================

void build_scr_main() {
    // 1. SFONDO SEMPLICE (Tinta unita per testare i colori)
    // Se vedi questo colore, il display funziona.
    lv_obj_set_style_bg_color(scr_main, lv_color_hex(0x0000FF), 0); // BLU
    lv_obj_set_style_bg_opa(scr_main, LV_OPA_COVER, 0);
    
    // 2. CONTENITORE CENTRALE
    lv_obj_t *center_panel = lv_obj_create(scr_main);
    lv_obj_set_size(center_panel, 400, 300);
    lv_obj_center(center_panel);
    lv_obj_set_style_bg_color(center_panel, lv_color_hex(0x000000), 0); // Nero
    lv_obj_set_style_border_color(center_panel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(center_panel, 2, 0);

    // 3. OROLOGIO (Testo Gigante)
    ui_lbl_hour = lv_label_create(center_panel);
    lv_obj_set_style_text_font(ui_lbl_hour, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_hour, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(ui_lbl_hour, "00");
    lv_obj_align(ui_lbl_hour, LV_ALIGN_CENTER, -60, -20);

    ui_lbl_dots = lv_label_create(center_panel);
    lv_obj_set_style_text_font(ui_lbl_dots, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_dots, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(ui_lbl_dots, ":");
    lv_obj_align(ui_lbl_dots, LV_ALIGN_CENTER, 0, -20);

    ui_lbl_min = lv_label_create(center_panel);
    lv_obj_set_style_text_font(ui_lbl_min, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_min, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(ui_lbl_min, "00");
    lv_obj_align(ui_lbl_min, LV_ALIGN_CENTER, 60, -20);

    // 4. DATA
    lbl_date = lv_label_create(center_panel);
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_date, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(lbl_date, LV_ALIGN_CENTER, 0, 40);
    lv_label_set_text(lbl_date, "Waiting Sync...");

    /* TUTTO IL RESTO E' RIMOSSO PER TEST
       - Niente colonna destra
       - Niente meteo
       - Niente tasti manuali
    */
}

// Funzioni dummy per evitare errori di link
void build_scr_caldaia() { scr_caldaia = lv_obj_create(NULL); }
void build_scr_impegni() { scr_impegni = lv_obj_create(NULL); }
void build_scr_setup() { scr_setup = lv_obj_create(NULL); }

void ui_init_all() {
    scr_main = lv_obj_create(NULL);
    build_scr_main();
    lv_scr_load(scr_main);
}

void update_current_weather(String temp, String desc, String iconCode) {
    // Disabilitato per test
}

void update_forecast_item(int index, String day, String temp, String iconCode) {
    // Disabilitato per test
}

void update_ui() {
    lv_obj_t *act = lv_scr_act();
    if (act != scr_main) return;

    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);

    if (timeinfo->tm_year > 120) {
        char bufHour[4];
        char bufMin[4];
        strftime(bufHour, sizeof(bufHour), "%H", timeinfo);
        strftime(bufMin, sizeof(bufMin), "%M", timeinfo);
        
        if(ui_lbl_hour) lv_label_set_text(ui_lbl_hour, bufHour);
        if(ui_lbl_min) lv_label_set_text(ui_lbl_min, bufMin);

        if(ui_lbl_dots) {
            if (timeinfo->tm_sec % 2 == 0) lv_obj_set_style_text_opa(ui_lbl_dots, LV_OPA_COVER, 0);
            else lv_obj_set_style_text_opa(ui_lbl_dots, LV_OPA_0, 0);
        }

        char dateBuf[32];
        strftime(dateBuf, sizeof(dateBuf), "%a %d %B", timeinfo);
        // Uppercase first char hack
        if(dateBuf[0] >= 'a' && dateBuf[0] <= 'z') dateBuf[0] -= 32;
        
        if(lbl_date) lv_label_set_text(lbl_date, dateBuf);
    }
}

// Stub functions needed by other files but not used in this test
void create_forecast_box(lv_obj_t *parent, int index) {}
void create_home_button(lv_obj_t *parent) {}
void render_weather_icon(lv_obj_t *parent, String code) {}