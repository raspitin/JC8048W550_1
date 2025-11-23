#include <lvgl.h>
#include "ui.h"
#include "thermostat.h"
#include <WiFi.h>
#include <time.h>

extern Thermostat thermo;

// Widget UI
lv_obj_t *scr_main;
lv_obj_t *arc_target;
lv_obj_t *lbl_cur_temp;
lv_obj_t *lbl_target_temp;
lv_obj_t *lbl_status;
lv_obj_t *lbl_clock;
lv_obj_t *lbl_touch_debug;

// Widget Setup WiFi
lv_obj_t *obj_wifi_alert;
lv_obj_t *lbl_wifi_msg;
lv_obj_t *lbl_wifi_ip;
lv_obj_t *qr_code;

static void arc_event_cb(lv_event_t * e) {
    lv_obj_t * arc = (lv_obj_t*)lv_event_get_target(e);
    int32_t value = lv_arc_get_value(arc);
    thermo.setTarget((float)value);
    lv_label_set_text_fmt(lbl_target_temp, "%d°C", value);
}

void create_nest_ui() {
    scr_main = lv_scr_act();
    lv_obj_set_style_bg_color(scr_main, lv_color_hex(0x1C1E26), 0);

    // --- HEADER: Orologio ---
    lbl_clock = lv_label_create(scr_main);
    lv_obj_set_style_text_font(lbl_clock, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_clock, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_clock, LV_ALIGN_TOP_RIGHT, -20, 10);
    lv_label_set_text(lbl_clock, "--:--");

    // --- BODY: Arco ---
    arc_target = lv_arc_create(scr_main);
    lv_obj_set_size(arc_target, 380, 380);
    lv_arc_set_rotation(arc_target, 135);
    lv_arc_set_bg_angles(arc_target, 0, 270);
    lv_arc_set_range(arc_target, 10, 30);
    lv_arc_set_value(arc_target, (int)thermo.getTarget());
    lv_obj_center(arc_target);
    lv_obj_set_style_arc_width(arc_target, 25, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_target, 25, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_target, lv_color_hex(0x2A2D37), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_target, lv_color_hex(0xF1C40F), LV_PART_INDICATOR);
    lv_obj_add_flag(arc_target, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(arc_target, arc_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // --- Temperatura Attuale (Centro) ---
    lbl_cur_temp = lv_label_create(scr_main);
    lv_obj_set_style_text_font(lbl_cur_temp, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(lbl_cur_temp, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_cur_temp, LV_ALIGN_CENTER, 0, -30);
    lv_label_set_text(lbl_cur_temp, "--.-°");

    // --- Target (Sotto la temp) ---
    lbl_target_temp = lv_label_create(scr_main);
    lv_obj_set_style_text_font(lbl_target_temp, &lv_font_montserrat_24, 0);
    lv_obj_align(lbl_target_temp, LV_ALIGN_CENTER, 0, 30);
    lv_label_set_text_fmt(lbl_target_temp, "%d°C", (int)thermo.getTarget());
    lv_obj_set_style_text_color(lbl_target_temp, lv_color_hex(0xA0A4AB), 0);

    // --- FOOTER: Stato ---
    lbl_status = lv_label_create(scr_main);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_status, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_label_set_text(lbl_status, "AVVIO...");
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xA0A4AB), 0);

    // --- DEBUG: Touch ---
    lbl_touch_debug = lv_label_create(scr_main);
    lv_obj_set_style_text_font(lbl_touch_debug, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_touch_debug, lv_color_hex(0x00FF00), 0);
    lv_obj_align(lbl_touch_debug, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_label_set_text(lbl_touch_debug, "");

    // --- LAYER POPUP SETUP WIFI ---
    obj_wifi_alert = lv_obj_create(scr_main);
    lv_obj_set_size(obj_wifi_alert, 700, 400);
    lv_obj_center(obj_wifi_alert);
    lv_obj_set_style_bg_color(obj_wifi_alert, lv_color_hex(0x2A2D37), 0);
    lv_obj_set_style_border_color(obj_wifi_alert, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(obj_wifi_alert, 4, 0);
    lv_obj_add_flag(obj_wifi_alert, LV_OBJ_FLAG_HIDDEN); // Nascondi all'avvio

    // Titolo popup
    lbl_wifi_msg = lv_label_create(obj_wifi_alert);
    lv_obj_set_style_text_font(lbl_wifi_msg, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_wifi_msg, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(lbl_wifi_msg, "SETUP RICHIESTO");
    lv_obj_align(lbl_wifi_msg, LV_ALIGN_TOP_MID, 0, 10);

    // QR Code
    qr_code = lv_qrcode_create(obj_wifi_alert);
    lv_qrcode_set_size(qr_code, 180);
    lv_qrcode_set_dark_color(qr_code, lv_color_hex(0x000000));
    lv_qrcode_set_light_color(qr_code, lv_color_hex(0xFFFFFF));
    lv_obj_set_style_border_color(qr_code, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(qr_code, 5, 0);
    lv_obj_center(qr_code);

    // SSID Label
    lbl_wifi_ip = lv_label_create(obj_wifi_alert);
    lv_obj_set_style_text_font(lbl_wifi_ip, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_wifi_ip, lv_color_hex(0xF1C40F), 0);
    lv_obj_align(lbl_wifi_ip, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_label_set_text(lbl_wifi_ip, "In attesa..."); 
}

void ui_show_setup_screen(const char* ssid, const char* pass) {
    if (!obj_wifi_alert) return;
    
    // Mostra il pannello
    lv_obj_remove_flag(obj_wifi_alert, LV_OBJ_FLAG_HIDDEN);
    
    // Aggiorna testi
    lv_label_set_text(lbl_wifi_msg, "SCANSIONA PER CONFIGURARE");
    lv_label_set_text_fmt(lbl_wifi_ip, "Connettiti a WiFi AP: %s", ssid);

    // Genera dati QR
    char qr_data[128];
    if (pass && strlen(pass) > 0) {
        snprintf(qr_data, sizeof(qr_data), "WIFI:S:%s;T:WPA;P:%s;;", ssid, pass);
    } else {
        snprintf(qr_data, sizeof(qr_data), "WIFI:S:%s;T:nopass;;", ssid);
    }
    
    // Aggiorna QR Code
    lv_qrcode_update(qr_code, qr_data, strlen(qr_data));
    
    // Forza aggiornamento grafico
    for(int i=0; i<10; i++) {
        lv_timer_handler();
        delay(20);
    }
}

void ui_hide_setup_screen() {
    if (obj_wifi_alert) lv_obj_add_flag(obj_wifi_alert, LV_OBJ_FLAG_HIDDEN);
}

void update_ui() {
    if (WiFi.status() == WL_CONNECTED) {
        ui_hide_setup_screen();
        
        if(thermo.isHeatingState()) {
            lv_obj_set_style_arc_color(arc_target, lv_color_hex(0xFF4500), LV_PART_INDICATOR);
            lv_label_set_text(lbl_status, "RISCALDAMENTO");
            lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xFF4500), 0);
        } else {
            lv_obj_set_style_arc_color(arc_target, lv_color_hex(0x3A8DFF), LV_PART_INDICATOR);
            lv_label_set_text(lbl_status, "STANDBY");
            lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xA0A4AB), 0);
        }
    } 

    lv_label_set_text_fmt(lbl_cur_temp, "%.1f°", thermo.getCurrentTemp());

    struct tm timeinfo;
    if(getLocalTime(&timeinfo)){
        char timeStringBuff[10]; 
        strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &timeinfo);
        lv_label_set_text(lbl_clock, timeStringBuff);
    }
    
    // Debug touch
    lv_indev_t * indev = lv_indev_get_next(NULL);
    if(indev) {
        lv_point_t p;
        lv_indev_get_point(indev, &p);
        lv_label_set_text_fmt(lbl_touch_debug, "Touch: X=%d Y=%d", p.x, p.y);
    }
}

void ui_log(String text) {
    Serial.println(text); 
}