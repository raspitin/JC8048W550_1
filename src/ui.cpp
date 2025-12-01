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
lv_obj_t *lbl_cur_temp_big; 
lv_obj_t *lbl_cur_desc;
lv_obj_t *cont_cur_icon;    

// Array previsioni
lv_obj_t *forecast_days[5];
lv_obj_t *forecast_temps[5];
lv_obj_t *forecast_icon_containers[5];

// Widget Setup
lv_obj_t *lbl_setup_ssid;
lv_obj_t *lbl_setup_ip;
lv_obj_t *lbl_setup_gw;

// Widget Bottone Manuale
lv_obj_t *btn_manual_toggle;
lv_obj_t *lbl_manual_toggle;

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
//  PROTOTIPI (Dichiarazioni anticipate)
// ============================================================================

void clear_icon(lv_obj_t *parent);
lv_obj_t* draw_circle(lv_obj_t *parent, int w, int h, int x, int y, uint32_t color);
void draw_cloud_shape(lv_obj_t *parent, int x_offset, int y_offset, uint32_t color);

void draw_icon_sun(lv_obj_t *parent);
void draw_icon_cloud(lv_obj_t *parent);
void draw_icon_partly_cloudy(lv_obj_t *parent);
void draw_icon_rain(lv_obj_t *parent);
void draw_icon_snow(lv_obj_t *parent);
void draw_icon_thunder(lv_obj_t *parent);
void render_weather_icon(lv_obj_t *parent, String code); 

void create_forecast_box(lv_obj_t *parent, int index);
void create_home_button(lv_obj_t *parent);

// ============================================================================
//  FUNZIONI GRAFICHE (IMPLEMENTAZIONI)
// ============================================================================

void clear_icon(lv_obj_t *parent) { lv_obj_clean(parent); }

lv_obj_t* draw_circle(lv_obj_t *parent, int w, int h, int x, int y, uint32_t color) {
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, w, h);
    lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(c, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_align(c, LV_ALIGN_CENTER, x, y);
    return c;
}

void draw_icon_sun(lv_obj_t *parent) {
    clear_icon(parent);
    lv_obj_t *rays = lv_obj_create(parent);
    lv_obj_set_size(rays, 55, 55);
    lv_obj_set_style_radius(rays, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(rays, 0, 0);
    lv_obj_set_style_border_color(rays, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_style_border_width(rays, 2, 0);
    lv_obj_center(rays);
    draw_circle(parent, 35, 35, 0, 0, 0xF1C40F);
}

void draw_cloud_shape(lv_obj_t *parent, int x_offset, int y_offset, uint32_t color) {
    draw_circle(parent, 25, 25, -12 + x_offset, 5 + y_offset, color); 
    draw_circle(parent, 25, 25, 12 + x_offset, 5 + y_offset, color);  
    draw_circle(parent, 35, 35, 0 + x_offset, -5 + y_offset, color);  
}

void draw_icon_cloud(lv_obj_t *parent) {
    clear_icon(parent);
    draw_cloud_shape(parent, 0, 0, 0xBDC3C7);
}

void draw_icon_partly_cloudy(lv_obj_t *parent) {
    clear_icon(parent);
    draw_circle(parent, 30, 30, 10, -10, 0xF1C40F);
    draw_cloud_shape(parent, -5, 5, 0xFFFFFF);
}

void draw_icon_rain(lv_obj_t *parent) {
    clear_icon(parent);
    for(int i=0; i<3; i++) {
        lv_obj_t *drop = lv_obj_create(parent);
        lv_obj_set_size(drop, 4, 10);
        lv_obj_set_style_bg_color(drop, lv_color_hex(0x3498DB), 0);
        lv_obj_set_style_radius(drop, 2, 0);
        lv_obj_set_style_border_width(drop, 0, 0);
        lv_obj_align(drop, LV_ALIGN_CENTER, (i*12) - 12, 15);
    }
    draw_cloud_shape(parent, 0, -5, 0x7F8C8D); 
}

void draw_icon_thunder(lv_obj_t *parent) {
    clear_icon(parent);
    lv_obj_t *bolt = lv_obj_create(parent);
    lv_obj_set_size(bolt, 8, 25);
    lv_obj_set_style_bg_color(bolt, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_style_transform_rotation(bolt, 200, 0);
    lv_obj_set_style_border_width(bolt, 0, 0);
    lv_obj_align(bolt, LV_ALIGN_CENTER, 0, 15);
    draw_cloud_shape(parent, 0, -5, 0x555555);
}

void draw_icon_snow(lv_obj_t *parent) {
    clear_icon(parent);
    for(int i=0; i<3; i++) {
        draw_circle(parent, 6, 6, (i*12) - 12, 15, 0xFFFFFF);
    }
    draw_cloud_shape(parent, 0, -5, 0xBDC3C7);
}

void render_weather_icon(lv_obj_t *parent, String code) {
    if (code == "01d" || code == "01n") draw_icon_sun(parent);
    else if (code == "02d" || code == "02n") draw_icon_partly_cloudy(parent);
    else if (code.startsWith("09") || code.startsWith("10")) draw_icon_rain(parent);
    else if (code.startsWith("11")) draw_icon_thunder(parent);
    else if (code.startsWith("13")) draw_icon_snow(parent);
    else draw_icon_cloud(parent);
}

// ============================================================================
//  NAVIGAZIONE E EVENTI
// ============================================================================

static void nav_event_cb(lv_event_t * e) {
    lv_obj_t * target_screen = (lv_obj_t *)lv_event_get_user_data(e);
    if(target_screen) {
        // IMPORTANTE: Usa LV_SCR_LOAD_ANIM_NONE
        // Le animazioni FADE su schermi 800x480 consumano troppa RAM e causano crash (schermo bianco)
        lv_scr_load_anim(target_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }
}

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
    lv_msgbox_add_text(mbox1, "Vuoi cancellare le impostazioni WiFi e riavviare?\nDovrai riconfigurare il dispositivo.");
    
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
//  LOGICA PULSANTE MANUALE
// ============================================================================

void update_manual_toggle_btn() {
    if(!btn_manual_toggle || !lbl_manual_toggle) return;

    if (thermo.isHeatingState()) {
        lv_obj_set_style_bg_color(btn_manual_toggle, lv_color_hex(0xE67E22), 0); // Arancio Caldo
        lv_label_set_text(lbl_manual_toggle, "Che caldo!!!");
    } else {
        lv_obj_set_style_bg_color(btn_manual_toggle, lv_color_hex(0x3498DB), 0); // Blu Freddo
        lv_label_set_text(lbl_manual_toggle, "Brr che freddo!!!");
    }
}

static void manual_toggle_cb(lv_event_t * e) {
    if (thermo.isHeatingState()) {
        #ifdef TARGET_HEAT_OFF
        thermo.setTarget(TARGET_HEAT_OFF);
        #else
        thermo.setTarget(16.0);
        #endif
        thermo.stopHeating();
    } else {
        #ifdef TARGET_HEAT_ON
        thermo.setTarget(TARGET_HEAT_ON);
        #else
        thermo.setTarget(22.0);
        #endif
        thermo.startHeating();
    }
    update_manual_toggle_btn();
}

// ============================================================================
//  EDITOR CALDAIA
// ============================================================================

static void editor_save_cb(lv_event_t * e) {
    int h = lv_roller_get_selected(roller_hour);
    int m = lv_roller_get_selected(roller_min) * 15;
    
    configManager.data.weekSchedule[edit_day_idx].slots[edit_slot_idx].hour = h;
    configManager.data.weekSchedule[edit_day_idx].slots[edit_slot_idx].minute = m;
    configManager.data.weekSchedule[edit_day_idx].slots[edit_slot_idx].temp = temp_editing;
    configManager.saveConfig();

    lv_label_set_text_fmt(lbl_slot_time[edit_day_idx][edit_slot_idx], "%02d:%02d", h, m);
    lv_label_set_text_fmt(lbl_slot_temp[edit_day_idx][edit_slot_idx], "%.1f°C", temp_editing);
    lv_obj_delete(win_editor);
}

static void editor_cancel_cb(lv_event_t * e) {
    lv_obj_delete(win_editor);
}

static void temp_btn_cb(lv_event_t * e) {
    int val = (int)(intptr_t)lv_event_get_user_data(e);
    temp_editing += (val * 0.5);
    if (temp_editing < 5.0) temp_editing = 5.0;
    if (temp_editing > 30.0) temp_editing = 30.0;
    lv_label_set_text_fmt(lbl_edit_temp, "%.1f°C", temp_editing);
}

void open_schedule_editor(int day, int slot) {
    edit_day_idx = day;
    edit_slot_idx = slot;
    ScheduleSlot curr = configManager.data.weekSchedule[day].slots[slot];
    temp_editing = curr.temp;

    win_editor = lv_obj_create(lv_scr_act());
    lv_obj_set_size(win_editor, 500, 350);
    lv_obj_center(win_editor);
    lv_obj_set_style_bg_color(win_editor, lv_color_hex(0x202020), 0);
    lv_obj_set_style_border_color(win_editor, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_style_border_width(win_editor, 2, 0);

    lv_obj_t *title = lv_label_create(win_editor);
    lv_label_set_text_fmt(title, "Modifica Fascia %d", slot + 1);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *cont_time = lv_obj_create(win_editor);
    lv_obj_set_size(cont_time, 200, 150);
    lv_obj_align(cont_time, LV_ALIGN_LEFT_MID, 10, -20);
    lv_obj_set_style_bg_opa(cont_time, 0, 0);
    lv_obj_set_style_border_width(cont_time, 0, 0);
    
    roller_hour = lv_roller_create(cont_time);
    lv_roller_set_options(roller_hour, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(roller_hour, 3);
    lv_obj_align(roller_hour, LV_ALIGN_LEFT_MID, 0, 0);
    lv_roller_set_selected(roller_hour, curr.hour, LV_ANIM_OFF);

    roller_min = lv_roller_create(cont_time);
    lv_roller_set_options(roller_min, "00\n15\n30\n45", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(roller_min, 3);
    lv_obj_align(roller_min, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_roller_set_selected(roller_min, curr.minute / 15, LV_ANIM_OFF);

    lv_obj_t *cont_temp = lv_obj_create(win_editor);
    lv_obj_set_size(cont_temp, 200, 150);
    lv_obj_align(cont_temp, LV_ALIGN_RIGHT_MID, -10, -20);
    lv_obj_set_style_bg_opa(cont_temp, 0, 0);
    lv_obj_set_style_border_width(cont_temp, 0, 0);

    lv_obj_t *btn_minus = lv_button_create(cont_temp);
    lv_obj_set_size(btn_minus, 50, 50);
    lv_obj_align(btn_minus, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(btn_minus, temp_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)-1);
    lv_obj_t *lbl_m = lv_label_create(btn_minus); lv_label_set_text(lbl_m, "-"); lv_obj_center(lbl_m);

    lbl_edit_temp = lv_label_create(cont_temp);
    lv_obj_set_style_text_font(lbl_edit_temp, &lv_font_montserrat_36, 0);
    lv_label_set_text_fmt(lbl_edit_temp, "%.1f°C", curr.temp);
    lv_obj_align(lbl_edit_temp, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_plus = lv_button_create(cont_temp);
    lv_obj_set_size(btn_plus, 50, 50);
    lv_obj_align(btn_plus, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(btn_plus, temp_btn_cb, LV_EVENT_CLICKED, (void*)(intptr_t)1);
    lv_obj_t *lbl_p = lv_label_create(btn_plus); lv_label_set_text(lbl_p, "+"); lv_obj_center(lbl_p);

    lv_obj_t *btn_save = lv_button_create(win_editor);
    lv_obj_set_size(btn_save, 120, 50);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x27AE60), 0);
    lv_obj_add_event_cb(btn_save, editor_save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ls = lv_label_create(btn_save); lv_label_set_text(ls, "SALVA"); lv_obj_center(ls);

    lv_obj_t *btn_cancel = lv_button_create(win_editor);
    lv_obj_set_size(btn_cancel, 120, 50);
    lv_obj_align(btn_cancel, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0xC0392B), 0);
    lv_obj_add_event_cb(btn_cancel, editor_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lc = lv_label_create(btn_cancel); lv_label_set_text(lc, "ANNULLA"); lv_obj_center(lc);
}

static void slot_click_cb(lv_event_t * e) {
    int data = (int)(intptr_t)lv_event_get_user_data(e);
    open_schedule_editor(data / 10, data % 10);
}

// ============================================================================
//  FUNZIONE MANCANTE: CREATE FORECAST BOX (Implementazione)
// ============================================================================

void create_forecast_box(lv_obj_t *parent, int index) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 110, 140); 
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_80, 0); 
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_radius(cont, 10, 0);
    
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont, 5, 0);
    
    forecast_days[index] = lv_label_create(cont);
    lv_obj_set_style_text_font(forecast_days[index], &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(forecast_days[index], lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(forecast_days[index], "--");

    forecast_icon_containers[index] = lv_obj_create(cont);
    lv_obj_set_size(forecast_icon_containers[index], 60, 50);
    lv_obj_set_style_bg_opa(forecast_icon_containers[index], 0, 0);
    lv_obj_set_style_border_width(forecast_icon_containers[index], 0, 0);
    lv_obj_remove_flag(forecast_icon_containers[index], LV_OBJ_FLAG_SCROLLABLE);

    forecast_temps[index] = lv_label_create(cont);
    lv_obj_set_style_text_font(forecast_temps[index], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(forecast_temps[index], lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(forecast_temps[index], "--°");
}

// ============================================================================
//  BUILDERS PAGINE
// ============================================================================

void build_scr_caldaia() {
    lv_obj_set_style_bg_color(scr_caldaia, lv_color_hex(0x100505), 0); 

    lv_obj_t *tabview = lv_tabview_create(scr_caldaia);
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_TOP);
    lv_obj_set_size(tabview, 800, 480);
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0x100505), 0);

    lv_obj_t * tab_btns = lv_tabview_get_tab_bar(tabview);
    lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x201010), 0);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0xFFFFFF), LV_PART_ITEMS | LV_STATE_CHECKED);
    
    const char* dayNames[] = {"LUN", "MAR", "MER", "GIO", "VEN", "SAB", "DOM"};

    for(int d=0; d<7; d++) {
        lv_obj_t * tab = lv_tabview_add_tab(tabview, dayNames[d]);
        lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(tab, 10, 0);
        lv_obj_set_style_pad_row(tab, 10, 0);

        lv_obj_t * l = lv_label_create(tab);
        lv_label_set_text_fmt(l, "Programma %s", dayNames[d]);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(l, lv_color_hex(0xF1C40F), 0);

        for(int s=0; s<4; s++) {
            lv_obj_t * row = lv_obj_create(tab);
            lv_obj_set_size(row, 700, 70);
            lv_obj_set_style_bg_color(row, lv_color_hex(0x2A2D37), 0);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(row, slot_click_cb, LV_EVENT_CLICKED, (void*)(intptr_t)((d*10)+s));

            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_hor(row, 30, 0);

            lbl_slot_time[d][s] = lv_label_create(row);
            lv_obj_set_style_text_font(lbl_slot_time[d][s], &lv_font_montserrat_24, 0);
            lv_obj_set_style_text_color(lbl_slot_time[d][s], lv_color_hex(0xFFFFFF), 0);
            
            lbl_slot_temp[d][s] = lv_label_create(row);
            lv_obj_set_style_text_font(lbl_slot_temp[d][s], &lv_font_montserrat_24, 0);
            lv_obj_set_style_text_color(lbl_slot_temp[d][s], lv_color_hex(0xF1C40F), 0);

            ScheduleSlot slotData = configManager.data.weekSchedule[d].slots[s];
            lv_label_set_text_fmt(lbl_slot_time[d][s], "%02d:%02d", slotData.hour, slotData.minute);
            lv_label_set_text_fmt(lbl_slot_temp[d][s], "%.1f°C", slotData.temp);
        }
    }
    create_home_button(scr_caldaia);
}

void build_scr_impegni() {
    lv_obj_set_style_bg_color(scr_impegni, lv_color_hex(0x051005), 0); 

    lv_obj_t *title = lv_label_create(scr_impegni);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(title, "Impegni");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    lv_obj_t *desc = lv_label_create(scr_impegni);
    lv_label_set_text(desc, "Lista Impegni / Memo\n(Feature Futura)");
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(desc, lv_color_hex(0xAAAAAA), 0);
    lv_obj_center(desc);

    create_home_button(scr_impegni);
}

void build_scr_setup() {
    lv_obj_set_style_bg_color(scr_setup, lv_color_hex(0x1C1E26), 0);

    lv_obj_t *title = lv_label_create(scr_setup);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(title, "Impostazioni");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *panel = lv_obj_create(scr_setup);
    lv_obj_set_size(panel, 600, 200);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x2A2D37), 0);
    lv_obj_set_style_border_width(panel, 0, 0);

    lbl_setup_ssid = lv_label_create(panel);
    lv_obj_set_style_text_color(lbl_setup_ssid, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(lbl_setup_ssid, "SSID: --");

    lbl_setup_ip = lv_label_create(panel);
    lv_obj_set_style_text_color(lbl_setup_ip, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(lbl_setup_ip, "IP: --");
    
    lbl_setup_gw = lv_label_create(panel);
    lv_obj_set_style_text_color(lbl_setup_gw, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(lbl_setup_gw, "Gateway: --");

    lv_obj_t *btn_reset = lv_button_create(scr_setup);
    lv_obj_set_size(btn_reset, 200, 60);
    lv_obj_align(btn_reset, LV_ALIGN_BOTTOM_LEFT, 50, -50);
    lv_obj_set_style_bg_color(btn_reset, lv_color_hex(0xFF0000), 0); 
    lv_obj_add_event_cb(btn_reset, btn_reset_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_rst = lv_label_create(btn_reset);
    lv_label_set_text(lbl_rst, "RESET WIFI");
    lv_obj_set_style_text_font(lbl_rst, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_rst);

    create_home_button(scr_setup);
}

void build_scr_main() {
    lv_obj_set_style_bg_color(scr_main, lv_color_hex(0x050505), 0);
    lv_obj_set_style_bg_grad_color(scr_main, lv_color_hex(0x001030), 0);
    lv_obj_set_style_bg_grad_dir(scr_main, LV_GRAD_DIR_VER, 0);
    
    lv_obj_t *col_left = lv_obj_create(scr_main);
    lv_obj_set_size(col_left, 650, 480);
    lv_obj_set_style_bg_opa(col_left, 0, 0);
    lv_obj_set_style_border_width(col_left, 0, 0);
    lv_obj_set_style_pad_all(col_left, 0, 0);
    lv_obj_align(col_left, LV_ALIGN_LEFT_MID, 0, 0);

    // OROLOGIO FLEX
    lv_obj_t *cont_clock = lv_obj_create(col_left);
    lv_obj_set_size(cont_clock, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(cont_clock, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont_clock, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_clock, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(cont_clock, 0, 0);
    lv_obj_set_style_border_width(cont_clock, 0, 0);
    lv_obj_set_style_pad_all(cont_clock, 0, 0);
    lv_obj_set_style_pad_column(cont_clock, 5, 0);
    lv_obj_align(cont_clock, LV_ALIGN_TOP_LEFT, 20, 20);

    ui_lbl_hour = lv_label_create(cont_clock);
    lv_obj_set_style_text_font(ui_lbl_hour, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_hour, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(ui_lbl_hour, "00");

    ui_lbl_dots = lv_label_create(cont_clock);
    lv_obj_set_style_text_font(ui_lbl_dots, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_dots, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(ui_lbl_dots, ":");

    ui_lbl_min = lv_label_create(cont_clock);
    lv_obj_set_style_text_font(ui_lbl_min, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(ui_lbl_min, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(ui_lbl_min, "00");

    lbl_date = lv_label_create(col_left);
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_date, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(lbl_date, LV_ALIGN_TOP_LEFT, 20, 70);
    lv_label_set_text(lbl_date, "---");

    // BOTTONE MANUALE
    btn_manual_toggle = lv_button_create(col_left);
    lv_obj_set_size(btn_manual_toggle, 220, 60);
    lv_obj_align(btn_manual_toggle, LV_ALIGN_TOP_RIGHT, -20, 50); 
    lv_obj_add_event_cb(btn_manual_toggle, manual_toggle_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn_manual_toggle, lv_color_hex(0x3498DB), 0); 
    
    lbl_manual_toggle = lv_label_create(btn_manual_toggle);
    lv_obj_set_style_text_font(lbl_manual_toggle, &lv_font_montserrat_20, 0);
    lv_label_set_text(lbl_manual_toggle, "Brr che freddo!!!");
    lv_obj_center(lbl_manual_toggle);

    lv_obj_t *cont_current = lv_obj_create(col_left);
    lv_obj_set_size(cont_current, 600, 150);
    lv_obj_set_style_bg_opa(cont_current, 0, 0);
    lv_obj_set_style_border_width(cont_current, 0, 0);
    lv_obj_align(cont_current, LV_ALIGN_TOP_LEFT, 10, 130);
    lv_obj_set_flex_flow(cont_current, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_current, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont_current, 20, 0);

    cont_cur_icon = lv_obj_create(cont_current);
    lv_obj_set_size(cont_cur_icon, 100, 100);
    lv_obj_set_style_bg_opa(cont_cur_icon, 0, 0);
    lv_obj_set_style_border_width(cont_cur_icon, 0, 0);
    
    lbl_cur_temp_big = lv_label_create(cont_current);
    lv_obj_set_style_text_font(lbl_cur_temp_big, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(lbl_cur_temp_big, lv_color_hex(0xF1C40F), 0);
    lv_label_set_text(lbl_cur_temp_big, "--°");

    lbl_cur_desc = lv_label_create(cont_current);
    lv_obj_set_style_text_font(lbl_cur_desc, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_cur_desc, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(lbl_cur_desc, 300); 
    lv_obj_set_style_text_align(lbl_cur_desc, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(lbl_cur_desc, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(lbl_cur_desc, "---");

    lv_obj_t *bot_section = lv_obj_create(col_left);
    lv_obj_set_size(bot_section, 630, 180); 
    lv_obj_set_style_bg_opa(bot_section, 0, 0);
    lv_obj_set_style_border_width(bot_section, 0, 0);
    lv_obj_align(bot_section, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_layout(bot_section, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bot_section, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bot_section, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for(int i=0; i<5; i++) {
        create_forecast_box(bot_section, i);
    }

    // --- COLONNA DESTRA ---
    lv_obj_t *col_right = lv_obj_create(scr_main);
    lv_obj_set_size(col_right, 150, 480);
    lv_obj_set_style_bg_color(col_right, lv_color_hex(0x151515), 0);
    lv_obj_set_style_border_side(col_right, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(col_right, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(col_right, 2, 0);
    lv_obj_set_style_radius(col_right, 0, 0);
    lv_obj_align(col_right, LV_ALIGN_RIGHT_MID, 0, 0);
    
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

    create_menu_btn("CALDAIA", LV_SYMBOL_CHARGE, nav_event_cb, scr_caldaia, 0xE67E22);
    create_menu_btn("SETUP", LV_SYMBOL_SETTINGS, nav_event_cb, scr_setup, 0x7F8C8D);
    create_menu_btn("IMPEGNI", LV_SYMBOL_LIST, nav_event_cb, scr_impegni, 0x27AE60);
}

void ui_init_all() {
    // 1. Allocazione placeholder
    scr_main = lv_obj_create(NULL);
    scr_caldaia = lv_obj_create(NULL);
    scr_setup = lv_obj_create(NULL);
    scr_impegni = lv_obj_create(NULL);

    // 2. Costruzione
    build_scr_caldaia();
    build_scr_impegni();
    build_scr_setup();
    build_scr_main();

    // 3. Avvio
    lv_scr_load(scr_main);
}

void update_current_weather(String temp, String desc, String iconCode) {
    if(lbl_cur_temp_big) lv_label_set_text_fmt(lbl_cur_temp_big, "%s°C", temp.c_str());
    if(lbl_cur_desc) lv_label_set_text(lbl_cur_desc, desc.c_str());
    if(cont_cur_icon) render_weather_icon(cont_cur_icon, iconCode);
}

void update_forecast_item(int index, String day, String temp, String iconCode) {
    if(index < 0 || index >= 5) return;
    lv_label_set_text(forecast_days[index], day.c_str());
    lv_label_set_text_fmt(forecast_temps[index], "%s°", temp.c_str());
    render_weather_icon(forecast_icon_containers[index], iconCode);
}

void update_ui() {
    lv_obj_t *act = lv_scr_act();
    if (act != scr_main && act != scr_setup) return;

    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);

    if (act == scr_main) update_manual_toggle_btn();

    if (act == scr_main && timeinfo->tm_year > 120) {
        char bufHour[4];
        char bufMin[4];
        strftime(bufHour, sizeof(bufHour), "%H", timeinfo);
        strftime(bufMin, sizeof(bufMin), "%M", timeinfo);
        
        if(ui_lbl_hour && strcmp(lv_label_get_text(ui_lbl_hour), bufHour) != 0) lv_label_set_text(ui_lbl_hour, bufHour);
        if(ui_lbl_min && strcmp(lv_label_get_text(ui_lbl_min), bufMin) != 0) lv_label_set_text(ui_lbl_min, bufMin);

        if(ui_lbl_dots) {
            if (timeinfo->tm_sec % 2 == 0) lv_obj_set_style_text_opa(ui_lbl_dots, LV_OPA_COVER, 0);
            else lv_obj_set_style_text_opa(ui_lbl_dots, LV_OPA_0, 0);
        }

        char dateBuf[32];
        strftime(dateBuf, sizeof(dateBuf), "%a %d %B", timeinfo);
        dateBuf[0] = toupper(dateBuf[0]);
        if(lbl_date) lv_label_set_text(lbl_date, dateBuf);
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