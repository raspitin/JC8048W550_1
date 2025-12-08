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

// DICHIARAZIONE IMMAGINE (Assicurati che il file logo_splash.c sia in src/)
LV_IMG_DECLARE(logo_splash);

// --- OGGETTI SPLASH SCREEN ---
lv_obj_t *scr_splash = NULL;
lv_obj_t *lbl_splash_status = NULL;

// --- SCHERMATE PRINCIPALI ---
// Definite qui, ma accessibili ovunque grazie a ui.h (extern)
lv_obj_t *scr_main = NULL;
lv_obj_t *scr_program = NULL;
lv_obj_t *scr_setup = NULL;
lv_obj_t *scr_impegni = NULL;

// --- WIDGET HOME GLOBALI ---
lv_obj_t *ui_lbl_hour = NULL;
lv_obj_t *ui_lbl_min = NULL;
lv_obj_t *ui_lbl_dots = NULL;

// Info Interno
lv_obj_t *ui_lbl_temp_icon = NULL;
lv_obj_t *ui_lbl_temp_val = NULL;
lv_obj_t *ui_lbl_hum_icon = NULL;
lv_obj_t *ui_lbl_hum_val = NULL;

lv_obj_t *lbl_date;         

// --- WIDGET METEO ---
lv_obj_t *cont_weather_today_icon = NULL;
lv_obj_t *lbl_weather_today_val = NULL;
lv_obj_t *cont_weather_tmrw_icon = NULL;
lv_obj_t *lbl_weather_tmrw_val = NULL;

// Widget Setup
lv_obj_t *lbl_setup_ssid;
lv_obj_t *lbl_setup_ip;
lv_obj_t *lbl_setup_gw;

// --- WIDGET HOME (BOOST) ---
lv_obj_t *lbl_today_schedule; 
lv_obj_t *btn_boost;          
lv_obj_t *lbl_boost_status;   

static int boost_minutes_selection = 30;
static lv_obj_t *lbl_popup_minutes;

// --- WIDGET PROGRAMMAZIONE ---
lv_obj_t *tv_days;      
lv_obj_t *timelines[7]; 
lv_obj_t *lbl_intervals[7]; 
lv_obj_t *lbl_drag_info[7];
static lv_obj_t * checkboxes[7]; 
static int source_day_for_copy = 0; 

// --- WIDGET IMPEGNI ---
lv_obj_t *list_impegni; 

#define TIMELINE_PAD_X 25 
static int prev_drag_slot_idx = -1;   
static int anchor_drag_slot_idx = -1; 
static int last_updated_day = -1;

// ============================================================================
//  PROTOTIPI
// ============================================================================
void create_home_button(lv_obj_t *parent);
void create_copy_popup(int sourceDayIndex);
void refresh_intervals_display(int ui_idx); 
void render_weather_icon(lv_obj_t *parent, String code);
void update_main_info_label(bool force); 
void create_boost_popup();
void get_time_string_from_slot(int slot, char* buf);
void show_relay_error_popup();
void load_impegni_to_ui(); 

// ============================================================================
//  SPLASH SCREEN
// ============================================================================

void ui_show_splash() {
    scr_splash = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_splash, lv_color_hex(0x000000), 0); 

    // 1. LOGO CENTRALE
    lv_obj_t * img = lv_image_create(scr_splash);
    lv_image_set_src(img, &logo_splash);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, -50); 

    // 2. TITOLO (Distanziato dal logo)
    lv_obj_t * lbl_title = lv_label_create(scr_splash);
    lv_label_set_text(lbl_title, "Cronotermostato Smart V1.0");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_24, 0); 
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xFFFFFF), 0);
    // MODIFICA: Spostato da 30 a 60 per distanziarlo dal logo
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 60);

    // 3. CREDITS (Font +1 e Bianco Brillante)
    lv_obj_t * lbl_credits = lv_label_create(scr_splash);
    lv_label_set_text(lbl_credits, "Sviluppato da Andrea e Gemini IA 2025");
    // MODIFICA: Font 16 (invece di 14)
    lv_obj_set_style_text_font(lbl_credits, &lv_font_montserrat_16, 0);
    // MODIFICA: Colore Bianco Puro (invece di Grigio)
    lv_obj_set_style_text_color(lbl_credits, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_credits, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Label di stato
    lbl_splash_status = lv_label_create(scr_splash);
    lv_label_set_text(lbl_splash_status, "");
    lv_obj_set_style_text_color(lbl_splash_status, lv_color_hex(0xE67E22), 0);
    lv_obj_align(lbl_splash_status, LV_ALIGN_BOTTOM_MID, 0, -60);

    // FIX: Usa lv_screen_load per LVGL 9
    lv_screen_load(scr_splash); 
}

void ui_splash_config_mode() {
    if (!scr_splash) return;

    // 1. Testo
    lv_label_set_text(lbl_splash_status, "Continua con la configurazione sul\ntelefono inquadrando questo QR Code");
    lv_obj_set_style_text_align(lbl_splash_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lbl_splash_status, LV_ALIGN_BOTTOM_MID, 0, -40);

    // 2. QR CODE
    const char * qr_data = "WIFI:S:Termostato_Setup;T:nopass;;";
    
    // In LVGL 9 si crea solo l'oggetto
    lv_obj_t * qr = lv_qrcode_create(scr_splash);
    
    // Poi si impostano i parametri separatamente
    lv_qrcode_set_size(qr, 140);
    lv_qrcode_set_dark_color(qr, lv_color_hex(0x000000));
    lv_qrcode_set_light_color(qr, lv_color_hex(0xFFFFFF));
    
    lv_qrcode_update(qr, qr_data, strlen(qr_data));
    
    lv_obj_align(qr, LV_ALIGN_CENTER, 0, 90); 
    lv_obj_set_style_border_color(qr, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(qr, 4, 0);

    lv_timer_handler(); 
}

// --- CALLBACK DI NAVIGAZIONE CORRETTA ---
static void nav_event_cb(lv_event_t * e) {
    lv_obj_t * target_screen = (lv_obj_t *)lv_event_get_user_data(e);
    if(target_screen) {
        if(target_screen == scr_impegni) load_impegni_to_ui(); 
        // FIX: Rimosso lv_scr_load_anim che causava l'errore "too many arguments"
        lv_screen_load(target_screen);
    }
}


// --- CALLBACKS GENERALI ---

static void error_popup_close_cb(lv_event_t * e) {
    lv_obj_t * win = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_delete(win);
}

void show_relay_error_popup() {
    lv_obj_t * win = lv_obj_create(lv_scr_act());
    lv_obj_set_size(win, 450, 280);
    lv_obj_center(win);
    lv_obj_set_style_bg_color(win, lv_color_hex(0x440000), 0); 
    lv_obj_set_style_border_color(win, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(win, 3, 0);
    lv_obj_set_flex_flow(win, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(win, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * icon = lv_label_create(win);
    lv_label_set_text(icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xFFD700), 0);

    lv_obj_t * lbl = lv_label_create(win);
    lv_label_set_text(lbl, "ERRORE COMUNICAZIONE\n\nRelè non raggiungibile.\nControllare alimentazione\no riavviarlo manualmente.");
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(lbl, 400);

    lv_obj_t * btn = lv_button_create(win);
    lv_obj_set_size(btn, 150, 50);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x555555), 0);
    lv_obj_add_event_cb(btn, error_popup_close_cb, LV_EVENT_CLICKED, win);
    
    lv_obj_t * l_btn = lv_label_create(btn);
    lv_label_set_text(l_btn, "ESCI");
    lv_obj_set_style_text_color(l_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(l_btn);
}

// --- CALLBACKS BOOST ---

static void boost_plus_cb(lv_event_t * e) {
    boost_minutes_selection += 30;
    if(boost_minutes_selection > 480) boost_minutes_selection = 480;
    lv_label_set_text_fmt(lbl_popup_minutes, "%d min", boost_minutes_selection);
}

static void boost_minus_cb(lv_event_t * e) {
    boost_minutes_selection -= 30;
    if(boost_minutes_selection < 30) boost_minutes_selection = 30;
    lv_label_set_text_fmt(lbl_popup_minutes, "%d min", boost_minutes_selection);
}

static void boost_start_cb(lv_event_t * e) {
    lv_obj_t * win = (lv_obj_t *)lv_event_get_user_data(e);
    bool ok = thermo.startBoost(boost_minutes_selection);
    lv_obj_delete(win);
    update_ui(); 
    if (!ok) show_relay_error_popup();
}

static void boost_cancel_cb(lv_event_t * e) {
    lv_obj_t * win = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_delete(win);
}

void create_boost_popup() {
    boost_minutes_selection = 30; 
    lv_obj_t * win = lv_obj_create(lv_scr_act());
    lv_obj_set_size(win, 400, 320);
    lv_obj_center(win);
    lv_obj_set_style_bg_color(win, lv_color_hex(0x202020), 0);
    lv_obj_set_style_border_color(win, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_style_border_width(win, 2, 0);
    lv_obj_set_flex_flow(win, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(win, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * title = lv_label_create(win);
    lv_label_set_text(title, "Accensione Temporizzata");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);

    lv_obj_t * cont_sel = lv_obj_create(win);
    lv_obj_set_size(cont_sel, 300, 100);
    lv_obj_set_style_bg_opa(cont_sel, 0, 0);
    lv_obj_set_style_border_width(cont_sel, 0, 0);
    lv_obj_set_flex_flow(cont_sel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_sel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * btn_m = lv_button_create(cont_sel);
    lv_obj_set_size(btn_m, 60, 60);
    lv_obj_add_event_cb(btn_m, boost_minus_cb, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn_m), "-");

    lbl_popup_minutes = lv_label_create(cont_sel);
    lv_obj_set_style_text_font(lbl_popup_minutes, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(lbl_popup_minutes, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(lbl_popup_minutes, "30 min");

    lv_obj_t * btn_p = lv_button_create(cont_sel);
    lv_obj_set_size(btn_p, 60, 60);
    lv_obj_add_event_cb(btn_p, boost_plus_cb, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn_p), "+");

    lv_obj_t * cont_act = lv_obj_create(win);
    lv_obj_set_size(cont_act, 350, 70);
    lv_obj_set_style_bg_opa(cont_act, 0, 0);
    lv_obj_set_style_border_width(cont_act, 0, 0);
    lv_obj_set_flex_flow(cont_act, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_act, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * btn_ok = lv_button_create(cont_act);
    lv_obj_set_size(btn_ok, 140, 50);
    lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0xE67E22), 0); 
    lv_obj_add_event_cb(btn_ok, boost_start_cb, LV_EVENT_CLICKED, win);
    lv_label_set_text(lv_label_create(btn_ok), "AVVIA");

    lv_obj_t * btn_no = lv_button_create(cont_act);
    lv_obj_set_size(btn_no, 140, 50);
    lv_obj_set_style_bg_color(btn_no, lv_color_hex(0x555555), 0);
    lv_obj_add_event_cb(btn_no, boost_cancel_cb, LV_EVENT_CLICKED, win);
    lv_label_set_text(lv_label_create(btn_no), "ANNULLA");
}

static void btn_boost_click_cb(lv_event_t * e) {
    if (!thermo.isRelayOnline()) {
        show_relay_error_popup();
        return;
    }

    if(thermo.isHeatingState()) {
        thermo.toggleOverride();
    } else {
        create_boost_popup(); 
    }
}

// --- HELPER FUNCS ---

void get_time_string_from_slot(int slot, char* buf) {
    if(slot > 48) slot = 48;
    int h = slot / 2;
    int m = (slot % 2) * 30;
    sprintf(buf, "%02d:%02d", h, m);
}

void update_main_info_label(bool force) {
    if(!lbl_date) return;
    
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    int day_idx = timeinfo->tm_wday; 
    
    if (timeinfo->tm_year + 1900 < 2023) {
        lv_label_set_text(lbl_date, "In attesa di orario...");
        return;
    }
    
    if (!force && day_idx == last_updated_day) return;
    last_updated_day = day_idx;

    const char* days[] = {"Domenica", "Lunedi'", "Martedi'", "Mercoledi'", "Giovedi'", "Venerdi'", "Sabato"};
    const char* months[] = {"Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno", "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre"};
    
    String text = "Accensioni per oggi " + String(days[day_idx]) + " " + String(timeinfo->tm_mday) + " " + String(months[timeinfo->tm_mon]) + ":\n";

    uint64_t slots = configManager.data.weekSchedule[day_idx].timeSlots;
    
    if (slots == 0) {
        text += "- Nessuna (Spento)";
    } else {
        int i = 0;
        int count = 0;
        while(i < 48 && count < 3) {
            if ((slots >> i) & 1ULL) {
                int start = i;
                while(i < 48 && ((slots >> i) & 1ULL)) i++;
                int end = i;
                
                char bufS[6], bufE[6];
                get_time_string_from_slot(start, bufS);
                get_time_string_from_slot(end, bufE);
                
                text += "- " + String(bufS) + " - " + String(bufE) + "\n";
                count++;
            } else {
                i++;
            }
        }
        if(i < 48 && count >= 3) text += "...";
    }
    
    lv_label_set_text(lbl_date, text.c_str());
}

void render_weather_icon(lv_obj_t *parent, String code) {
    lv_obj_clean(parent);
    // Disegna icone semplici con cerchi colorati se non hai immagini
    if (code == "01d" || code == "01n") { // Sole
        lv_obj_t *c = lv_obj_create(parent); lv_obj_set_size(c, 30, 30); lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0); 
        lv_obj_set_style_bg_color(c, lv_color_hex(0xF1C40F), 0); lv_obj_center(c);
    } else if (code.startsWith("09") || code.startsWith("10")) { // Pioggia
        lv_obj_t *c = lv_obj_create(parent); lv_obj_set_size(c, 30, 30); lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0); 
        lv_obj_set_style_bg_color(c, lv_color_hex(0x3498DB), 0); lv_obj_center(c);
    } else { // Nuvole
        lv_obj_t *c = lv_obj_create(parent); lv_obj_set_size(c, 30, 30); lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0); 
        lv_obj_set_style_bg_color(c, lv_color_hex(0xBDC3C7), 0); lv_obj_center(c);
    }
}

// ... msgbox callbacks ...
static void msgbox_close_cb(lv_event_t * e) {
    lv_obj_t * mbox = (lv_obj_t *)lv_event_get_user_data(e);
    if(mbox) lv_msgbox_close(mbox);
}

static void msgbox_confirm_reset_cb(lv_event_t * e) {
    lv_obj_t * mbox = (lv_obj_t *)lv_event_get_user_data(e);
    if(mbox) lv_msgbox_close(mbox);
    wifi_reset_settings();
}

static void btn_reset_event_cb(lv_event_t * e) {
    lv_obj_t * mbox1 = lv_msgbox_create(scr_setup);
    lv_msgbox_add_title(mbox1, "Conferma Reset");
    lv_msgbox_add_text(mbox1, "Vuoi cancellare le impostazioni WiFi?");
    lv_obj_t * btn_yes = lv_msgbox_add_footer_button(mbox1, "SI, RESETTA");
    lv_obj_set_style_bg_color(btn_yes, lv_color_hex(0xFF0000), 0);
    lv_obj_add_event_cb(btn_yes, msgbox_confirm_reset_cb, LV_EVENT_CLICKED, mbox1);
    lv_obj_t * btn_no = lv_msgbox_add_footer_button(mbox1, "ANNULLA");
    lv_obj_add_event_cb(btn_no, msgbox_close_cb, LV_EVENT_CLICKED, mbox1);
    lv_obj_center(mbox1);
}

void create_home_button(lv_obj_t *parent) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 60, 60);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_HOME); 
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, nav_event_cb, LV_EVENT_CLICKED, scr_main);
}

// ============================================================================
//  LOGICA IMPEGNI (CARICAMENTO DA FILE CON FILTRO E PULIZIA)
// ============================================================================

void load_impegni_to_ui() {
    lv_obj_clean(list_impegni);
    
    if (!LittleFS.exists("/impegni.json")) {
        lv_obj_t * lbl = lv_label_create(list_impegni);
        lv_label_set_text(lbl, "Nessun impegno salvato.");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
        return;
    }

    File file = LittleFS.open("/impegni.json", "r");
    if (!file) return;

    DynamicJsonDocument doc(8192); 
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        lv_obj_t * lbl = lv_label_create(list_impegni);
        lv_label_set_text(lbl, "Errore caricamento impegni.");
        return;
    }

    time_t now;
    time(&now);
    struct tm *t_now = localtime(&now);
    
    struct tm t_start = *t_now;
    t_start.tm_hour = 0; t_start.tm_min = 0; t_start.tm_sec = 0;
    time_t ts_today = mktime(&t_start);
    time_t ts_limit = ts_today + (3 * 24 * 3600) + 86399; 

    JsonArray arr = doc.as<JsonArray>();
    DynamicJsonDocument newDoc(8192);
    JsonArray newArr = newDoc.to<JsonArray>();
    bool needSave = false;
    int visibleCount = 0;

    for (JsonVariant v : arr) {
        String imp = v.as<String>();
        if (imp.length() >= 10) {
            String datePart = imp.substring(0, 10); 
            struct tm t_imp = {0};
            int y, m, d;
            if (sscanf(datePart.c_str(), "%d/%d/%d", &y, &m, &d) == 3) {
                t_imp.tm_year = y - 1900;
                t_imp.tm_mon = m - 1;
                t_imp.tm_mday = d;
                t_imp.tm_hour = 12; 
                time_t ts_imp = mktime(&t_imp);
                
                if (ts_imp < ts_today) {
                    needSave = true;
                    continue; 
                } 
                
                newArr.add(imp);

                if (ts_imp <= ts_limit) {
                    char dateBuf[64];
                    snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d %s", d, m, imp.substring(11).c_str());
                    
                    lv_obj_t * btn = lv_list_add_btn(list_impegni, LV_SYMBOL_BULLET, dateBuf);
                    lv_obj_set_style_text_font(btn, &lv_font_montserrat_20, 0);
                    lv_obj_set_style_bg_color(btn, lv_color_hex(0x202020), 0);
                    lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
                    lv_obj_set_style_border_width(btn, 0, 0);
                    lv_obj_set_style_pad_all(btn, 15, 0);
                    visibleCount++;
                }
            } else {
                newArr.add(imp); 
            }
        }
    }

    if (visibleCount == 0) {
        lv_obj_t * lbl = lv_label_create(list_impegni);
        lv_label_set_text(lbl, "Nessun impegno imminente.");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x888888), 0);
        lv_obj_set_style_pad_all(lbl, 20, 0);
    }

    if (needSave) {
        File fileW = LittleFS.open("/impegni.json", "w");
        if (fileW) {
            serializeJson(newDoc, fileW);
            fileW.close();
        }
    }
}

void refresh_intervals_display(int ui_idx) {
    if(ui_idx < 0 || ui_idx >= 7) return;
    if(!lbl_intervals[ui_idx]) return;
    int config_idx = (ui_idx + 1) % 7;
    uint64_t slots = configManager.data.weekSchedule[config_idx].timeSlots;
    String text = "";
    int count = 0; int i = 0;
    while(i < 48 && count < 3) {
        if ((slots >> i) & 1ULL) {
            int start = i;
            while(i < 48 && ((slots >> i) & 1ULL)) i++;
            int end = i; 
            char bufStart[6], bufEnd[6];
            get_time_string_from_slot(start, bufStart);
            get_time_string_from_slot(end, bufEnd);
            count++;
            text += "Orario " + String(count) + ": " + String(bufStart) + " - " + String(bufEnd) + "\n";
        } else i++;
    }
    if (text == "") text = "Nessuna fascia oraria impostata";
    if (i < 48 && count >= 3) {
        bool more = false;
        for(int k=i; k<48; k++) if((slots >> k) & 1ULL) more = true;
        if(more) text += "...";
    }
    lv_label_set_text(lbl_intervals[ui_idx], text.c_str());
}

static void timeline_draw_event_cb(lv_event_t * e) {
    lv_obj_t * obj = (lv_obj_t*)lv_event_get_target(e);
    int day_idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_layer_t * layer = lv_event_get_layer(e);
    lv_area_t obj_coords; lv_obj_get_coords(obj, &obj_coords);
    int w = lv_obj_get_width(obj); int h = lv_obj_get_height(obj);
    int x_start_draw = obj_coords.x1 + TIMELINE_PAD_X;
    int w_draw = w - (2 * TIMELINE_PAD_X);
    int y_start = obj_coords.y1;
    uint64_t slots = configManager.data.weekSchedule[day_idx].timeSlots;
    float slot_w = (float)w_draw / 48.0;
    int available_h = h - 25; int bar_h = available_h * 0.4; int bar_y_offset = (available_h - bar_h) / 2 + 5; 
    lv_draw_rect_dsc_t dsc_bg; lv_draw_rect_dsc_init(&dsc_bg);
    dsc_bg.bg_color = lv_color_hex(0x333333); dsc_bg.bg_opa = LV_OPA_COVER; dsc_bg.radius = 4; 
    lv_area_t bg_area; bg_area.x1 = x_start_draw; bg_area.x2 = x_start_draw + w_draw - 1; bg_area.y1 = y_start + bar_y_offset; bg_area.y2 = bg_area.y1 + bar_h;
    lv_draw_rect(layer, &dsc_bg, &bg_area);
    lv_draw_rect_dsc_t dsc_on; lv_draw_rect_dsc_init(&dsc_on);
    dsc_on.bg_color = lv_color_hex(0x27AE60); dsc_on.bg_opa = LV_OPA_COVER; dsc_on.radius = 4;
    int i = 0;
    while(i < 48) {
        if ((slots >> i) & 1ULL) {
            int start_i = i; while(i < 48 && ((slots >> i) & 1ULL)) i++; int end_i = i; 
            int x1 = x_start_draw + (int)(start_i * slot_w); int x2 = x_start_draw + (int)(end_i * slot_w) - 1;
            lv_area_t on_area; on_area.x1 = x1; on_area.x2 = x2; on_area.y1 = y_start + bar_y_offset; on_area.y2 = on_area.y1 + bar_h;
            lv_draw_rect(layer, &dsc_on, &on_area);
        } else i++;
    }
    lv_draw_label_dsc_t label_dsc; lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = lv_color_hex(0xFFFFFF); label_dsc.font = &lv_font_montserrat_14; label_dsc.align = LV_TEXT_ALIGN_CENTER;
    for(int h_idx=0; h_idx<=24; h_idx+=4) {
        char buf[8]; lv_snprintf(buf, sizeof(buf), "%02d", h_idx); label_dsc.text = buf;
        int tick_x = x_start_draw + (int)(h_idx * 2 * slot_w);
        lv_area_t label_area; label_area.y1 = y_start + h - 20; label_area.y2 = y_start + h;
        if(h_idx == 0) { label_area.x1 = tick_x - 10; label_area.x2 = tick_x + 20; label_dsc.align = LV_TEXT_ALIGN_LEFT; } 
        else if(h_idx == 24) { label_area.x1 = tick_x - 20; label_area.x2 = tick_x + 10; label_dsc.align = LV_TEXT_ALIGN_RIGHT; } 
        else { label_area.x1 = tick_x - 15; label_area.x2 = tick_x + 15; label_dsc.align = LV_TEXT_ALIGN_CENTER; }
        lv_draw_label(layer, &label_dsc, &label_area);
    }
}

static void timeline_input_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = (lv_obj_t*)lv_event_get_target(e);
    int ui_idx = (int)(intptr_t)lv_event_get_user_data(e);
    int config_idx = (ui_idx + 1) % 7;
    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        prev_drag_slot_idx = -1; anchor_drag_slot_idx = -1;
        lv_label_set_text(lbl_drag_info[ui_idx], "Trascina per selezionare orari");
        refresh_intervals_display(ui_idx); configManager.saveConfig(); update_main_info_label(true); return;
    }
    if (code == LV_EVENT_PRESSED) { prev_drag_slot_idx = -1; anchor_drag_slot_idx = -1; }
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        lv_indev_t * indev = lv_indev_get_act(); if(!indev) return;
        lv_point_t p; lv_indev_get_point(indev, &p);
        lv_area_t coords; lv_obj_get_coords(obj, &coords);
        int w = lv_obj_get_width(obj); int w_draw = w - (2 * TIMELINE_PAD_X);
        int rel_x = p.x - coords.x1 - TIMELINE_PAD_X;
        if(rel_x < 0) rel_x = 0; if(rel_x >= w_draw) rel_x = w_draw - 1;
        int current_slot_idx = (rel_x * 48) / w_draw;
        if(current_slot_idx < 0) current_slot_idx = 0; if(current_slot_idx > 47) current_slot_idx = 47;
        if(anchor_drag_slot_idx == -1) anchor_drag_slot_idx = current_slot_idx;
        int drag_start = (anchor_drag_slot_idx < current_slot_idx) ? anchor_drag_slot_idx : current_slot_idx;
        int drag_end = (anchor_drag_slot_idx < current_slot_idx) ? current_slot_idx : anchor_drag_slot_idx;
        char bufS[6], bufE[6]; get_time_string_from_slot(drag_start, bufS); get_time_string_from_slot(drag_end + 1, bufE);
        lv_label_set_text_fmt(lbl_drag_info[ui_idx], "Selezionando: %s - %s", bufS, bufE);
        int start_fill = current_slot_idx; int end_fill = current_slot_idx;
        if (code == LV_EVENT_PRESSING && prev_drag_slot_idx != -1) {
            if (current_slot_idx > prev_drag_slot_idx) { start_fill = prev_drag_slot_idx; end_fill = current_slot_idx; } else { start_fill = current_slot_idx; end_fill = prev_drag_slot_idx; }
        }
        uint64_t *slots = &configManager.data.weekSchedule[config_idx].timeSlots;
        bool changed = false;
        int min_x_inv = 99999; int max_x_inv = -99999;
        float slot_w = (float)w_draw / 48.0; int x_base = coords.x1 + TIMELINE_PAD_X;
        for(int i = start_fill; i <= end_fill; i++) {
            if ( !((*slots >> i) & 1ULL) ) {
                *slots |= (1ULL << i); changed = true;
                int sx1 = x_base + (int)(i * slot_w); int sx2 = x_base + (int)((i + 1) * slot_w);
                if(sx1 < min_x_inv) min_x_inv = sx1; if(sx2 > max_x_inv) max_x_inv = sx2;
            }
        }
        if (changed) {
            lv_area_t inv_area; inv_area.x1 = min_x_inv; inv_area.x2 = max_x_inv; inv_area.y1 = coords.y1; inv_area.y2 = coords.y2;
            lv_obj_invalidate_area(obj, &inv_area);
        }
        prev_drag_slot_idx = current_slot_idx;
    }
}
static void clear_prog_cb(lv_event_t * e) {
    int ui_idx = (int)(intptr_t)lv_event_get_user_data(e);
    int config_idx = (ui_idx + 1) % 7;
    configManager.data.weekSchedule[config_idx].timeSlots = 0;
    if(timelines[ui_idx]) lv_obj_invalidate(timelines[ui_idx]);
    refresh_intervals_display(ui_idx);
    time_t now; time(&now); struct tm *timeinfo = localtime(&now);
    if (timeinfo->tm_wday == config_idx) update_main_info_label(true);
    configManager.saveConfig();
}
static void copy_confirm_cb(lv_event_t * e) {
    lv_obj_t * win = (lv_obj_t *)lv_event_get_user_data(e);
    uint64_t pattern = configManager.data.weekSchedule[source_day_for_copy].timeSlots;
    for(int i=0; i<7; i++) {
        if(lv_obj_has_state(checkboxes[i], LV_STATE_CHECKED)) {
            int config_idx = (i + 1) % 7;
            configManager.data.weekSchedule[config_idx].timeSlots = pattern;
            if(timelines[i]) lv_obj_invalidate(timelines[i]);
            refresh_intervals_display(i);
        }
    }
    update_main_info_label(true);
    configManager.saveConfig();
    lv_obj_delete(win);
}
static void copy_cancel_cb(lv_event_t * e) { lv_obj_t * win = (lv_obj_t *)lv_event_get_user_data(e); lv_obj_delete(win); }
void create_copy_popup(int uiDayIndex) {
    int configDayIndex = (uiDayIndex + 1) % 7; source_day_for_copy = configDayIndex;
    lv_obj_t * win = lv_obj_create(lv_scr_act()); lv_obj_set_size(win, 500, 350); lv_obj_center(win);
    lv_obj_set_style_bg_color(win, lv_color_hex(0x202020), 0); lv_obj_set_style_border_color(win, lv_color_hex(0xFFFFFF), 0); lv_obj_set_style_border_width(win, 2, 0);
    lv_obj_set_flex_flow(win, LV_FLEX_FLOW_COLUMN); lv_obj_set_flex_align(win, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(win, lv_color_hex(0x202020), 0); lv_obj_set_style_border_color(win, lv_color_hex(0xFFFFFF), 0); lv_obj_set_style_border_width(win, 2, 0);
    lv_obj_set_flex_flow(win, LV_FLEX_FLOW_COLUMN); lv_obj_set_flex_align(win, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(win, lv_color_hex(0x202020), 0); lv_obj_set_style_border_color(win, lv_color_hex(0xFFFFFF), 0); lv_obj_set_style_border_width(win, 2, 0);
    lv_obj_set_flex_flow(win, LV_FLEX_FLOW_COLUMN); lv_obj_set_flex_align(win, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t * title = lv_label_create(win); lv_label_set_text(title, "Copia su altri giorni:");
    lv_obj_t * cont_checks = lv_obj_create(win); lv_obj_set_size(cont_checks, 450, 180); lv_obj_set_flex_flow(cont_checks, LV_FLEX_FLOW_ROW_WRAP);
    const char* names[] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};
    for(int i=0; i<7; i++) {
        checkboxes[i] = lv_checkbox_create(cont_checks); lv_checkbox_set_text(checkboxes[i], names[i]);
        if(i != uiDayIndex) lv_obj_add_state(checkboxes[i], LV_STATE_CHECKED);
    }
    lv_obj_t * cont_btns = lv_obj_create(win); lv_obj_set_size(cont_btns, 450, 60); lv_obj_set_flex_flow(cont_btns, LV_FLEX_FLOW_ROW); lv_obj_set_flex_align(cont_btns, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t * btn_ok = lv_button_create(cont_btns); lv_obj_set_style_bg_color(btn_ok, lv_color_hex(0x27AE60), 0);
    lv_obj_add_event_cb(btn_ok, copy_confirm_cb, LV_EVENT_CLICKED, win);
    lv_obj_t * l1 = lv_label_create(btn_ok); lv_label_set_text(l1, "CONFERMA");
    lv_obj_t * btn_no = lv_button_create(cont_btns); lv_obj_set_style_bg_color(btn_no, lv_color_hex(0xC0392B), 0);
    lv_obj_add_event_cb(btn_no, copy_cancel_cb, LV_EVENT_CLICKED, win);
    lv_obj_t * l2 = lv_label_create(btn_no); lv_label_set_text(l2, "ANNULLA");
}
static void open_copy_popup_cb(lv_event_t * e) { int ui_idx = (int)(intptr_t)lv_event_get_user_data(e); create_copy_popup(ui_idx); }

// ============================================================================
//  BUILDERS PAGINE
// ============================================================================

void build_scr_program() {
    lv_obj_set_style_bg_color(scr_program, lv_color_hex(0x101015), 0); 
    tv_days = lv_tabview_create(scr_program); lv_tabview_set_tab_bar_position(tv_days, LV_DIR_TOP);
    lv_obj_set_size(tv_days, 800, 480); lv_obj_set_style_bg_color(tv_days, lv_color_hex(0x101015), 0);
    lv_obj_t * tab_btns = lv_tabview_get_tab_bar(tv_days);
    lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x202020), 0); lv_obj_set_style_text_font(tab_btns, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0xAAAAAA), 0); lv_obj_set_style_text_color(tab_btns, lv_color_hex(0xFFFFFF), LV_PART_ITEMS | LV_STATE_CHECKED); 
    lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0xE67E22), LV_PART_ITEMS | LV_STATE_CHECKED); 
    lv_obj_remove_flag(tab_btns, LV_OBJ_FLAG_SCROLLABLE);

    const char* dayNames[] = {"LUN", "MAR", "MER", "GIO", "VEN", "SAB", "DOM"};
    for(int i=0; i<7; i++) {
        lv_obj_t * tab = lv_tabview_add_tab(tv_days, dayNames[i]);
        int config_idx = (i + 1) % 7;
        lv_obj_remove_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

        lbl_drag_info[i] = lv_label_create(tab); lv_label_set_text(lbl_drag_info[i], "Trascina per selezionare orari");
        lv_obj_set_style_text_color(lbl_drag_info[i], lv_color_hex(0xFFD700), 0); lv_obj_align(lbl_drag_info[i], LV_ALIGN_TOP_MID, 0, 10);

        timelines[i] = lv_obj_create(tab); lv_obj_set_size(timelines[i], 750, 120);
        lv_obj_align(timelines[i], LV_ALIGN_TOP_MID, 0, 40); 
        lv_obj_set_style_bg_color(timelines[i], lv_color_hex(0x000000), 0);
        lv_obj_set_style_border_width(timelines[i], 2, 0); lv_obj_set_style_border_color(timelines[i], lv_color_hex(0x555555), 0);
        lv_obj_remove_flag(timelines[i], LV_OBJ_FLAG_SCROLLABLE); lv_obj_remove_flag(timelines[i], LV_OBJ_FLAG_GESTURE_BUBBLE); 
        lv_obj_add_flag(timelines[i], LV_OBJ_FLAG_CLICKABLE); lv_obj_set_scroll_dir(timelines[i], LV_DIR_NONE);

        lv_obj_set_user_data(timelines[i], (void*)(intptr_t)i); 
        lv_obj_add_event_cb(timelines[i], timeline_draw_event_cb, LV_EVENT_DRAW_MAIN, (void*)(intptr_t)config_idx);
        lv_obj_add_event_cb(timelines[i], timeline_input_event_cb, LV_EVENT_PRESSING, (void*)(intptr_t)i);
        lv_obj_add_event_cb(timelines[i], timeline_input_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_add_event_cb(timelines[i], timeline_input_event_cb, LV_EVENT_RELEASED, (void*)(intptr_t)i);
        lv_obj_add_event_cb(timelines[i], timeline_input_event_cb, LV_EVENT_PRESS_LOST, (void*)(intptr_t)i);

        lbl_intervals[i] = lv_label_create(tab); lv_obj_set_width(lbl_intervals[i], 700);
        lv_obj_set_style_text_align(lbl_intervals[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(lbl_intervals[i], lv_color_hex(0xFFFFFF), 0); 
        lv_obj_align(lbl_intervals[i], LV_ALIGN_TOP_MID, 0, 170); 
        refresh_intervals_display(i);

        lv_obj_t * cont_actions = lv_obj_create(tab); lv_obj_set_size(cont_actions, 750, 80);
        lv_obj_align(cont_actions, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_flex_flow(cont_actions, LV_FLEX_FLOW_ROW); lv_obj_set_flex_align(cont_actions, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_opa(cont_actions, 0, 0); lv_obj_set_style_border_width(cont_actions, 0, 0);

        lv_obj_t * btn_clr = lv_button_create(cont_actions); lv_obj_set_size(btn_clr, 200, 50);
        lv_obj_set_style_bg_color(btn_clr, lv_color_hex(0xC0392B), 0);
        lv_obj_add_event_cb(btn_clr, clear_prog_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_t * l_clr = lv_label_create(btn_clr); lv_label_set_text(l_clr, "CANCELLA GIORNO");

        lv_obj_t * btn_cpy = lv_button_create(cont_actions); lv_obj_set_size(btn_cpy, 200, 50);
        lv_obj_set_style_bg_color(btn_cpy, lv_color_hex(0x2980B9), 0);
        lv_obj_add_event_cb(btn_cpy, open_copy_popup_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);
        lv_obj_t * l_cpy = lv_label_create(btn_cpy); lv_label_set_text(l_cpy, "COPIA SU...");
    }
    lv_obj_t * content = lv_tabview_get_content(tv_days);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    create_home_button(scr_program);
}

void build_scr_impegni() {
    lv_obj_set_style_bg_color(scr_impegni, lv_color_hex(0x051005), 0); 
    lv_obj_t *title = lv_label_create(scr_impegni); lv_obj_set_style_text_font(title, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0); lv_label_set_text(title, "Impegni");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    
    // LISTA IMPEGNI
    list_impegni = lv_list_create(scr_impegni);
    lv_obj_set_size(list_impegni, 700, 350);
    lv_obj_align(list_impegni, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_color(list_impegni, lv_color_hex(0x101015), 0);
    lv_obj_set_style_border_width(list_impegni, 0, 0);
    
    create_home_button(scr_impegni);
}

void build_scr_setup() {
    lv_obj_set_style_bg_color(scr_setup, lv_color_hex(0x1C1E26), 0);
    lv_obj_set_style_text_color(scr_setup, lv_color_hex(0xFFFFFF), 0);
    lv_obj_t *title = lv_label_create(scr_setup); lv_obj_set_style_text_font(title, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0); lv_label_set_text(title, "Impostazioni");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_t *panel = lv_obj_create(scr_setup); lv_obj_set_size(panel, 600, 200);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, -40); lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x2A2D37), 0); lv_obj_set_style_border_width(panel, 0, 0);
    lbl_setup_ssid = lv_label_create(panel); lv_obj_set_style_text_color(lbl_setup_ssid, lv_color_hex(0xFFFFFF), 0); lv_label_set_text(lbl_setup_ssid, "SSID: --");
    lbl_setup_ip = lv_label_create(panel); lv_obj_set_style_text_color(lbl_setup_ip, lv_color_hex(0xFFFFFF), 0); lv_label_set_text(lbl_setup_ip, "IP: --");
    lbl_setup_gw = lv_label_create(panel); lv_obj_set_style_text_color(lbl_setup_gw, lv_color_hex(0xFFFFFF), 0); lv_label_set_text(lbl_setup_gw, "Gateway: --");
    lv_obj_t *btn_reset = lv_button_create(scr_setup); lv_obj_set_size(btn_reset, 200, 60);
    lv_obj_align(btn_reset, LV_ALIGN_BOTTOM_LEFT, 50, -50); lv_obj_set_style_bg_color(btn_reset, lv_color_hex(0xFF0000), 0); 
    lv_obj_add_event_cb(btn_reset, btn_reset_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_rst = lv_label_create(btn_reset); lv_label_set_text(lbl_rst, "RESET WIFI");
    lv_obj_set_style_text_font(lbl_rst, &lv_font_montserrat_20, 0); lv_obj_center(lbl_rst);
    create_home_button(scr_setup);
}

void build_scr_main() {
    lv_obj_set_style_bg_color(scr_main, lv_color_hex(0x050505), 0);
    lv_obj_set_style_bg_grad_color(scr_main, lv_color_hex(0x001030), 0); lv_obj_set_style_bg_grad_dir(scr_main, LV_GRAD_DIR_VER, 0);
    
    lv_obj_t *col_left = lv_obj_create(scr_main); lv_obj_set_size(col_left, 650, 480);
    lv_obj_set_style_bg_opa(col_left, 0, 0); lv_obj_set_style_border_width(col_left, 0, 0);
    lv_obj_set_style_pad_all(col_left, 0, 0); lv_obj_align(col_left, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_remove_flag(col_left, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cont_clock = lv_obj_create(col_left); lv_obj_set_size(cont_clock, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(cont_clock, LV_LAYOUT_FLEX); lv_obj_set_flex_flow(cont_clock, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_clock, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(cont_clock, 0, 0); lv_obj_set_style_border_width(cont_clock, 0, 0);
    lv_obj_set_style_pad_all(cont_clock, 0, 0); lv_obj_set_style_pad_column(cont_clock, 5, 0);
    lv_obj_align(cont_clock, LV_ALIGN_TOP_LEFT, 20, 20);

    ui_lbl_hour = lv_label_create(cont_clock); lv_obj_set_style_text_font(ui_lbl_hour, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_hour, lv_color_hex(0xFFFFFF), 0); lv_label_set_text(ui_lbl_hour, "00");

    ui_lbl_dots = lv_label_create(cont_clock); lv_obj_set_style_text_font(ui_lbl_dots, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_dots, lv_color_hex(0xFFFFFF), 0); lv_label_set_text(ui_lbl_dots, ":");

    ui_lbl_min = lv_label_create(cont_clock); lv_obj_set_style_text_font(ui_lbl_min, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_min, lv_color_hex(0xFFFFFF), 0); lv_label_set_text(ui_lbl_min, "00");

    // --- SPAZIATORE E INFO SENSORI ---
    lv_obj_t *spacer = lv_obj_create(cont_clock);
    lv_obj_set_size(spacer, 20, 1);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_style_bg_opa(spacer, 0, 0);

    // --- ICONA TEMPERATURA ---
    ui_lbl_temp_icon = lv_label_create(cont_clock);
    lv_obj_set_style_text_font(ui_lbl_temp_icon, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_temp_icon, lv_color_hex(0xF1C40F), 0); 
    lv_label_set_text(ui_lbl_temp_icon, "T");

    // --- VALORE TEMPERATURA ---
    ui_lbl_temp_val = lv_label_create(cont_clock);
    lv_obj_set_style_text_font(ui_lbl_temp_val, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_temp_val, lv_color_hex(0xFFFFFF), 0); 
    lv_obj_set_width(ui_lbl_temp_val, 130);
    lv_obj_set_style_text_align(ui_lbl_temp_val, LV_TEXT_ALIGN_LEFT, 0); 
    lv_label_set_text(ui_lbl_temp_val, "--.-°C");

    // --- ICONA UMIDITÀ ---
    ui_lbl_hum_icon = lv_label_create(cont_clock);
    lv_obj_set_style_text_font(ui_lbl_hum_icon, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_hum_icon, lv_color_hex(0x3498DB), 0); 
    lv_label_set_text(ui_lbl_hum_icon, "H");

    // --- VALORE UMIDITÀ ---
    ui_lbl_hum_val = lv_label_create(cont_clock);
    lv_obj_set_style_text_font(ui_lbl_hum_val, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_hum_val, lv_color_hex(0xFFFFFF), 0); 
    lv_obj_set_width(ui_lbl_hum_val, 100);
    lv_obj_set_style_text_align(ui_lbl_hum_val, LV_TEXT_ALIGN_LEFT, 0); 
    lv_label_set_text(ui_lbl_hum_val, "--%");

    lbl_date = lv_label_create(col_left); 
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_date, lv_color_hex(0xAAAAAA), 0); 
    lv_obj_set_width(lbl_date, 600);
    lv_label_set_long_mode(lbl_date, LV_LABEL_LONG_WRAP);
    lv_obj_align(lbl_date, LV_ALIGN_TOP_LEFT, 20, 70);
    lv_label_set_text(lbl_date, "In attesa di dati...");

    // --- SEZIONE METEO A 2 GIORNI ---
    lv_obj_t *cont_weather_section = lv_obj_create(col_left); 
    lv_obj_set_size(cont_weather_section, 600, 160); // Aumentata altezza per 2 righe
    lv_obj_set_style_bg_opa(cont_weather_section, 0, 0); 
    lv_obj_set_style_border_width(cont_weather_section, 0, 0);
    lv_obj_align(cont_weather_section, LV_ALIGN_TOP_LEFT, 20, 200); // MODIFICA: Y=200 per spostare in basso
    lv_obj_set_flex_flow(cont_weather_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(cont_weather_section, 10, 0); // Spazio tra le righe
    lv_obj_remove_flag(cont_weather_section, LV_OBJ_FLAG_SCROLLABLE);

    // RIGA OGGI
    lv_obj_t *row_today = lv_obj_create(cont_weather_section);
    lv_obj_set_size(row_today, 580, 60);
    lv_obj_set_style_bg_opa(row_today, 0, 0); 
    lv_obj_set_style_border_width(row_today, 0, 0);
    lv_obj_set_flex_flow(row_today, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_today, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row_today, 10, 0);
    lv_obj_remove_flag(row_today, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *lbl_today = lv_label_create(row_today);
    lv_obj_set_style_text_font(lbl_today, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_today, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(lbl_today, 120); // AUMENTATO DA 80 A 120
    lv_obj_set_style_text_align(lbl_today, LV_TEXT_ALIGN_LEFT, 0); // ALLINEATO A SX
    lv_label_set_text(lbl_today, "Oggi:");

    cont_weather_today_icon = lv_obj_create(row_today);
    lv_obj_set_size(cont_weather_today_icon, 60, 60);
    lv_obj_set_style_bg_opa(cont_weather_today_icon, 0, 0);
    lv_obj_set_style_border_width(cont_weather_today_icon, 0, 0);

    lbl_weather_today_val = lv_label_create(row_today);
    lv_obj_set_style_text_font(lbl_weather_today_val, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_weather_today_val, lv_color_hex(0xFFD700), 0);
    lv_label_set_text(lbl_weather_today_val, "--°C");

    // RIGA DOMANI
    lv_obj_t *row_tmrw = lv_obj_create(cont_weather_section);
    lv_obj_set_size(row_tmrw, 580, 60);
    lv_obj_set_style_bg_opa(row_tmrw, 0, 0); 
    lv_obj_set_style_border_width(row_tmrw, 0, 0);
    lv_obj_set_flex_flow(row_tmrw, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_tmrw, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row_tmrw, 10, 0);
    lv_obj_remove_flag(row_tmrw, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_tmrw = lv_label_create(row_tmrw);
    lv_obj_set_style_text_font(lbl_tmrw, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_tmrw, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(lbl_tmrw, 120); // AUMENTATO DA 80 A 120
    lv_obj_set_style_text_align(lbl_tmrw, LV_TEXT_ALIGN_LEFT, 0); // ALLINEATO A SX
    lv_label_set_text(lbl_tmrw, "Domani:");

    cont_weather_tmrw_icon = lv_obj_create(row_tmrw);
    lv_obj_set_size(cont_weather_tmrw_icon, 60, 60);
    lv_obj_set_style_bg_opa(cont_weather_tmrw_icon, 0, 0);
    lv_obj_set_style_border_width(cont_weather_tmrw_icon, 0, 0);

    lbl_weather_tmrw_val = lv_label_create(row_tmrw);
    lv_obj_set_style_text_font(lbl_weather_tmrw_val, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_weather_tmrw_val, lv_color_hex(0xAAAAAA), 0);
    lv_label_set_text(lbl_weather_tmrw_val, "--°C");

    // Area Basso Sinistra (Pulsante BOOST)
    lv_obj_t *bot_section = lv_obj_create(col_left); lv_obj_set_size(bot_section, 630, 100); 
    lv_obj_set_style_bg_opa(bot_section, 0, 0); lv_obj_set_style_border_width(bot_section, 0, 0);
    lv_obj_align(bot_section, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    lv_obj_remove_flag(bot_section, LV_OBJ_FLAG_SCROLLABLE);
    
    btn_boost = lv_button_create(bot_section);
    lv_obj_set_size(btn_boost, 250, 70);
    lv_obj_center(btn_boost);
    lv_obj_add_event_cb(btn_boost, btn_boost_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn_boost, lv_color_hex(0x3498DB), 0); 
    
    lbl_boost_status = lv_label_create(btn_boost);
    lv_obj_set_style_text_font(lbl_boost_status, &lv_font_montserrat_24, 0);
    lv_label_set_text(lbl_boost_status, "Brr che freddo!!!"); 
    lv_obj_center(lbl_boost_status);

    // --- COLONNA DESTRA (MENU) ---
    lv_obj_t *col_right = lv_obj_create(scr_main); lv_obj_set_size(col_right, 150, 480);
    lv_obj_set_style_bg_color(col_right, lv_color_hex(0x151515), 0);
    lv_obj_set_style_border_side(col_right, LV_BORDER_SIDE_LEFT, 0); lv_obj_set_style_border_color(col_right, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(col_right, 2, 0); lv_obj_set_style_radius(col_right, 0, 0);
    lv_obj_align(col_right, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_remove_flag(col_right, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_set_flex_flow(col_right, LV_FLEX_FLOW_COLUMN);
    
    lv_obj_set_flex_flow(col_right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col_right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col_right, 30, 0);

    auto create_menu_btn = [&](const char* text, const char* symbol, lv_event_cb_t cb, lv_obj_t* user_data, uint32_t color) {
        lv_obj_t *btn = lv_button_create(col_right);
        lv_obj_set_size(btn, 120, 100);
        lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
        
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *lbl_sym = lv_label_create(btn);
        lv_label_set_text(lbl_sym, symbol);
        lv_obj_set_style_text_font(lbl_sym, &lv_font_montserrat_24, 0);

        lv_obj_t *lbl_txt = lv_label_create(btn);
        lv_label_set_text(lbl_txt, text);
        lv_obj_set_style_text_font(lbl_txt, &lv_font_montserrat_14, 0);
    };

    create_menu_btn("PROGRAM", LV_SYMBOL_LIST, nav_event_cb, scr_program, 0xE67E22);
    create_menu_btn("SETUP", LV_SYMBOL_SETTINGS, nav_event_cb, scr_setup, 0x7F8C8D);
    create_menu_btn("IMPEGNI", LV_SYMBOL_LIST, nav_event_cb, scr_impegni, 0x27AE60);
    
    update_main_info_label(true);
}

void ui_init_all() {
    scr_main = lv_obj_create(NULL);
    scr_program = lv_obj_create(NULL);
    scr_setup = lv_obj_create(NULL);
    scr_impegni = lv_obj_create(NULL);

    build_scr_program();
    build_scr_impegni();
    build_scr_setup();
    build_scr_main();

    lv_scr_load(scr_main);
}

// Aggiorna Meteo Oggi
void update_current_weather(String temp, String desc, String iconCode) {
    if(lbl_weather_today_val) lv_label_set_text_fmt(lbl_weather_today_val, "%s°C %s", temp.c_str(), desc.c_str());
    if(cont_weather_today_icon) render_weather_icon(cont_weather_today_icon, iconCode);
}

// Aggiorna Meteo Domani
void update_forecast_item(int index, String day, String temp, String desc, String iconCode) {
    if (index == 1) { // 1 = Domani
        // MODIFICA: Ora usa anche 'desc' nel formato
        if(lbl_weather_tmrw_val) lv_label_set_text_fmt(lbl_weather_tmrw_val, "%s°C %s", temp.c_str(), desc.c_str());
        if(cont_weather_tmrw_icon) render_weather_icon(cont_weather_tmrw_icon, iconCode);
    }
}

void update_ui() {
    lv_obj_t *act = lv_scr_act();
    if (act != scr_main && act != scr_setup) return;

    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);

    // Aggiorna Stato Boost
    if (act == scr_main) {
        
        // LOGICA COLORE PULSANTE
        if (!thermo.isRelayOnline()) {
            // RELÈ OFFLINE -> GRIGIO
            lv_obj_set_style_bg_color(btn_boost, lv_color_hex(0x555555), 0); 
            lv_label_set_text(lbl_boost_status, "Rele' Offline");
        } 
        else if (thermo.isBoostActive()) {
            // BOOST ATTIVO -> ARANCIO
            lv_obj_set_style_bg_color(btn_boost, lv_color_hex(0xE67E22), 0); 
            long rem = thermo.getBoostRemainingSeconds();
            int min = rem / 60;
            lv_label_set_text_fmt(lbl_boost_status, "Che caldo!!!\n-%d min", min);
        } 
        else if (thermo.isHeatingState()) {
            // RISCALDAMENTO PROGRAMMATO ATTIVO -> ROSSO
            lv_obj_set_style_bg_color(btn_boost, lv_color_hex(0xC0392B), 0); // Rosso
            lv_label_set_text(lbl_boost_status, "Che caldo...\n(Spegni)");
        }
        else {
            // STANDBY (o SPENTO MANUALMENTE) -> BLU
            // Il comportamento è identico: se clicchi, ti apre il popup per accendere.
            lv_obj_set_style_bg_color(btn_boost, lv_color_hex(0x3498DB), 0); 
            lv_label_set_text(lbl_boost_status, "Brr che freddo!!!");
        }

        // --- Aggiornamento info sensore interno ---
        if(ui_lbl_temp_val) {
            float t = thermo.getCurrentTemp();
            float h = thermo.getHumidity();
            // Aggiorna solo i valori, le icone sono statiche
            lv_label_set_text_fmt(ui_lbl_temp_val, "%.1f°C", t);
            lv_label_set_text_fmt(ui_lbl_hum_val, "%.0f%%", h);
        }
        
        update_main_info_label(false);
    }

    if (act == scr_main && timeinfo->tm_year > 120) {
        char bufHour[4]; char bufMin[4];
        strftime(bufHour, sizeof(bufHour), "%H", timeinfo); strftime(bufMin, sizeof(bufMin), "%M", timeinfo);
        if(ui_lbl_hour && strcmp(lv_label_get_text(ui_lbl_hour), bufHour) != 0) lv_label_set_text(ui_lbl_hour, bufHour);
        if(ui_lbl_min && strcmp(lv_label_get_text(ui_lbl_min), bufMin) != 0) lv_label_set_text(ui_lbl_min, bufMin);
        if(ui_lbl_dots) {
            if (timeinfo->tm_sec % 2 == 0) lv_obj_set_style_text_opa(ui_lbl_dots, LV_OPA_COVER, 0);
            else lv_obj_set_style_text_opa(ui_lbl_dots, LV_OPA_0, 0);
        }
    }

    if (act == scr_setup) {
        if (WiFi.status() == WL_CONNECTED) {
            lv_label_set_text_fmt(lbl_setup_ssid, "SSID: %s", WiFi.SSID().c_str());
            lv_label_set_text_fmt(lbl_setup_ip, "IP: %s", WiFi.localIP().toString().c_str());
            lv_label_set_text_fmt(lbl_setup_gw, "Gateway: %s", WiFi.gatewayIP().toString().c_str());
        } else {
            lv_label_set_text(lbl_setup_ssid, "SSID: Disconnesso");
        }
    }
}
