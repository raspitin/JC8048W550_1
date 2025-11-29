#include <lvgl.h>
#include "ui.h"
#include "network_manager.h" 
#include "config_manager.h"
#include <WiFi.h>
#include <time.h>

// Variabile esterna
#include "thermostat.h"
extern Thermostat thermo;

// --- WIDGET GLOBALI (Inizializzati a NULL per sicurezza) ---
lv_obj_t *scr_main = NULL;
lv_obj_t *scr_caldaia = NULL;
lv_obj_t *scr_setup = NULL;
lv_obj_t *scr_impegni = NULL;

lv_obj_t *ui_lbl_hour = NULL;
lv_obj_t *ui_lbl_min = NULL;
lv_obj_t *ui_lbl_dots = NULL;
lv_obj_t *lbl_date = NULL;
lv_obj_t *lbl_cur_temp_big = NULL; 
lv_obj_t *lbl_cur_desc = NULL;
lv_obj_t *cont_cur_icon = NULL;    

lv_obj_t *forecast_days[5];
lv_obj_t *forecast_temps[5];
lv_obj_t *forecast_icon_containers[5];

lv_obj_t *lbl_setup_ssid = NULL;
lv_obj_t *lbl_setup_ip = NULL;
lv_obj_t *lbl_setup_gw = NULL;

lv_obj_t *btn_manual_toggle = NULL;
lv_obj_t *lbl_manual_toggle = NULL;

// Widget Editor
int edit_day_idx = 0;
int edit_slot_idx = 0;
lv_obj_t *lbl_slot_time[7][4]; 
lv_obj_t *lbl_slot_temp[7][4];
lv_obj_t *win_editor = NULL;
lv_obj_t *roller_hour = NULL;
lv_obj_t *roller_min = NULL;
lv_obj_t *lbl_edit_temp = NULL;
float temp_editing = 20.0;

// --- CACHE PER EVITARE SFARFALLIO ---
// Memorizziamo lo stato precedente per aggiornare solo al cambiamento
static char prev_hour[4] = "";
static char prev_min[4] = "";
static char prev_date[32] = "";
static bool prev_dots_visible = true;
static bool prev_wifi_connected = false;
static bool prev_heating_state = false;
static bool first_run = true; // Flag per forzare il primo aggiornamento

// ... [QUI VANNO INSERITE TUTTE LE FUNZIONI GRAFICHE: draw_icon_..., create_forecast_box, ecc.] ...
// (Copia le funzioni grafiche draw_* e create_* dal file precedente, sono invariate)
// Per brevità non le ripeto qui, ma SONO NECESSARIE.
// Assicurati di includere: clear_icon, draw_circle, draw_icon_sun/cloud/rain/snow/thunder/partly, render_weather_icon
// e tutte le funzioni build_scr_...

// ... [FUNZIONI EVENTI E NAVIGAZIONE INVARIATE] ...

// ... [FUNZIONI EDITOR INVARIATE] ...

// ... [FUNZIONE ui_init_all INVARIATA] ...

// ============================================================================
//  LOGICA DI AGGIORNAMENTO OTTIMIZZATA (ANTI-SFARFALLIO)
// ============================================================================

void update_manual_toggle_btn() {
    if(!btn_manual_toggle || !lbl_manual_toggle) return;

    bool current_heating = thermo.isHeatingState();
    
    // Aggiorna SOLO se lo stato è cambiato
    if (current_heating != prev_heating_state || first_run) {
        if (current_heating) {
            lv_obj_set_style_bg_color(btn_manual_toggle, lv_color_hex(0xE67E22), 0); // Caldo
            lv_label_set_text(lbl_manual_toggle, "Che caldo!!!");
        } else {
            lv_obj_set_style_bg_color(btn_manual_toggle, lv_color_hex(0x3498DB), 0); // Freddo
            lv_label_set_text(lbl_manual_toggle, "Brr che freddo!!!");
        }
        prev_heating_state = current_heating;
    }
}

void update_current_weather(String temp, String desc, String iconCode) {
    // Qui usiamo un controllo diretto perché i dati arrivano da evento esterno (non loop)
    if(lbl_cur_temp_big) lv_label_set_text_fmt(lbl_cur_temp_big, "%s°C", temp.c_str());
    if(lbl_cur_desc) lv_label_set_text(lbl_cur_desc, desc.c_str());
    if(cont_cur_icon) render_weather_icon(cont_cur_icon, iconCode);
}

void update_forecast_item(int index, String day, String temp, String iconCode) {
    if(index < 0 || index >= 5) return;
    // Validazione puntatori array
    if(forecast_days[index]) lv_label_set_text(forecast_days[index], day.c_str());
    if(forecast_temps[index]) lv_label_set_text_fmt(forecast_temps[index], "%s°", temp.c_str());
    if(forecast_icon_containers[index]) render_weather_icon(forecast_icon_containers[index], iconCode);
}

void update_ui() {
    lv_obj_t *act = lv_scr_act();
    if (act != scr_main && act != scr_setup) return;

    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);

    // Aggiorna Bottone Manuale (con check interno)
    if (act == scr_main) update_manual_toggle_btn();

    // Aggiorna Orologio
    if (act == scr_main && timeinfo->tm_year > 120) {
        // Buffer temporanei
        char current_hour[4];
        char current_min[4];
        char current_date[32];
        
        strftime(current_hour, sizeof(current_hour), "%H", timeinfo);
        strftime(current_min, sizeof(current_min), "%M", timeinfo);
        
        // 1. Aggiorna ORA solo se cambiata
        if (ui_lbl_hour && strcmp(current_hour, prev_hour) != 0) {
            lv_label_set_text(ui_lbl_hour, current_hour);
            strcpy(prev_hour, current_hour);
        }

        // 2. Aggiorna MINUTI solo se cambiati
        if (ui_lbl_min && strcmp(current_min, prev_min) != 0) {
            lv_label_set_text(ui_lbl_min, current_min);
            strcpy(prev_min, current_min);
        }

        // 3. Lampeggio Due Punti (Ogni secondo)
        // Questo deve cambiare sempre, ma usiamo l'opacità per non invalidare il layout
        bool dots_visible = (timeinfo->tm_sec % 2 == 0);
        if (ui_lbl_dots && dots_visible != prev_dots_visible) {
            lv_obj_set_style_text_opa(ui_lbl_dots, dots_visible ? LV_OPA_COVER : LV_OPA_0, 0);
            prev_dots_visible = dots_visible;
        }

        // 4. Aggiorna DATA solo se cambiata
        strftime(current_date, sizeof(current_date), "%a %d %B", timeinfo);
        current_date[0] = toupper(current_date[0]); // Capitalizza
        
        if (lbl_date && strcmp(current_date, prev_date) != 0) {
            lv_label_set_text(lbl_date, current_date);
            strcpy(prev_date, current_date);
        }
    }

    // Aggiorna Pagina Setup (Solo se attiva)
    if (act == scr_setup) {
        bool wifi_connected = (WiFi.status() == WL_CONNECTED);
        
        // Aggiorna solo se lo stato di connessione cambia o al primo avvio
        if (wifi_connected != prev_wifi_connected || first_run) {
            if (wifi_connected) {
                if(lbl_setup_ssid) lv_label_set_text_fmt(lbl_setup_ssid, "SSID: %s", WiFi.SSID().c_str());
                if(lbl_setup_ip) lv_label_set_text_fmt(lbl_setup_ip, "IP: %s", WiFi.localIP().toString().c_str());
                if(lbl_setup_gw) lv_label_set_text_fmt(lbl_setup_gw, "Gateway: %s", WiFi.gatewayIP().toString().c_str());
            } else {
                if(lbl_setup_ssid) lv_label_set_text(lbl_setup_ssid, "SSID: Disconnesso");
            }
            prev_wifi_connected = wifi_connected;
        }
    }
    
    first_run = false; // Fine primo ciclo
}