#include <lvgl.h>
#include "ui.h"
#include <WiFi.h>
#include <time.h>

// --- WIDGET GLOBALI ---
// Orologio (Flex Container)
lv_obj_t *ui_lbl_hour = NULL;
lv_obj_t *ui_lbl_min = NULL;
lv_obj_t *ui_lbl_dots = NULL;

lv_obj_t *lbl_date;
lv_obj_t *lbl_current_temp;
lv_obj_t *lbl_current_desc;
lv_obj_t *lbl_ip;

// Array per i 5 giorni di previsione
lv_obj_t *forecast_days[5];
lv_obj_t *forecast_temps[5];
lv_obj_t *forecast_icon_containers[5];

// --- FUNZIONI DI DISEGNO ICONE AVANZATE ---
// Usiamo primitive LVGL per disegnare icone vettoriali composite

void clear_icon(lv_obj_t *parent) {
    lv_obj_clean(parent);
}

// Helper per disegnare un cerchio (parte di nuvola o sole)
lv_obj_t* draw_circle(lv_obj_t *parent, int w, int h, int x, int y, uint32_t color) {
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, w, h);
    lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(c, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_align(c, LV_ALIGN_CENTER, x, y);
    return c;
}

// SOLE: Cerchio centrale giallo + alone
void draw_icon_sun(lv_obj_t *parent) {
    clear_icon(parent);
    // Raggi/Alone (Cerchio trasparente con bordo spesso)
    lv_obj_t *rays = lv_obj_create(parent);
    lv_obj_set_size(rays, 55, 55);
    lv_obj_set_style_radius(rays, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(rays, 0, 0);
    lv_obj_set_style_border_color(rays, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_style_border_width(rays, 2, 0);
    lv_obj_set_style_border_side(rays, LV_BORDER_SIDE_FULL, 0); // Linea tratteggiata non supportata facilmente, usiamo solida
    lv_obj_center(rays);

    // Corpo sole
    draw_circle(parent, 35, 35, 0, 0, 0xF1C40F);
}

// NUVOLA: Tre cerchi sovrapposti
void draw_cloud_shape(lv_obj_t *parent, int x_offset, int y_offset, uint32_t color) {
    draw_circle(parent, 25, 25, -12 + x_offset, 5 + y_offset, color); // Sx
    draw_circle(parent, 25, 25, 12 + x_offset, 5 + y_offset, color);  // Dx
    draw_circle(parent, 35, 35, 0 + x_offset, -5 + y_offset, color);  // Centro Alto
}

// NUVOLOSO: Nuvola Grigia
void draw_icon_cloud(lv_obj_t *parent) {
    clear_icon(parent);
    draw_cloud_shape(parent, 0, 0, 0xBDC3C7); // Grigio chiaro
}

// POCO NUVOLOSO: Sole dietro, Nuvola davanti
void draw_icon_partly_cloudy(lv_obj_t *parent) {
    clear_icon(parent);
    // Sole (spostato in alto a destra)
    draw_circle(parent, 30, 30, 10, -10, 0xF1C40F);
    // Nuvola (spostata in basso a sinistra, Bianca)
    draw_cloud_shape(parent, -5, 5, 0xFFFFFF);
}

// PIOGGIA: Nuvola scura + Gocce
void draw_icon_rain(lv_obj_t *parent) {
    clear_icon(parent);
    // Gocce (Linee o piccoli rettangoli)
    for(int i=0; i<3; i++) {
        lv_obj_t *drop = lv_obj_create(parent);
        lv_obj_set_size(drop, 4, 10);
        lv_obj_set_style_bg_color(drop, lv_color_hex(0x3498DB), 0);
        lv_obj_set_style_radius(drop, 2, 0);
        lv_obj_set_style_border_width(drop, 0, 0);
        lv_obj_align(drop, LV_ALIGN_CENTER, (i*12) - 12, 15);
    }
    // Nuvola scura sopra
    draw_cloud_shape(parent, 0, -5, 0x7F8C8D); 
}

// TEMPORALE: Nuvola scura + Fulmine
void draw_icon_thunder(lv_obj_t *parent) {
    clear_icon(parent);
    // Fulmine (giallo) - simulato con un rettangolo ruotato (semplificato)
    lv_obj_t *bolt = lv_obj_create(parent);
    lv_obj_set_size(bolt, 8, 25);
    lv_obj_set_style_bg_color(bolt, lv_color_hex(0xF1C40F), 0);
    lv_obj_set_style_transform_rotation(bolt, 200, 0); // Ruotato
    lv_obj_set_style_border_width(bolt, 0, 0);
    lv_obj_align(bolt, LV_ALIGN_CENTER, 0, 15);

    draw_cloud_shape(parent, 0, -5, 0x555555); // Grigio scuro
}

// NEVE: Nuvola + pallini bianchi
void draw_icon_snow(lv_obj_t *parent) {
    clear_icon(parent);
    for(int i=0; i<3; i++) {
        draw_circle(parent, 6, 6, (i*12) - 12, 15, 0xFFFFFF);
    }
    draw_cloud_shape(parent, 0, -5, 0xBDC3C7);
}

void render_weather_icon(lv_obj_t *parent, String code) {
    if (code == "01d" || code == "01n") draw_icon_sun(parent);
    else if (code == "02d" || code == "02n") draw_icon_partly_cloudy(parent); // Poco nuvoloso
    else if (code.startsWith("09") || code.startsWith("10")) draw_icon_rain(parent);
    else if (code.startsWith("11")) draw_icon_thunder(parent);
    else if (code.startsWith("13")) draw_icon_snow(parent);
    else draw_icon_cloud(parent); // Default nuvoloso (03, 04, 50)
}

// --- LAYOUT ---

void create_forecast_box(lv_obj_t *parent, int index) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 130, 150); 
    lv_obj_set_style_bg_color(cont, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_80, 0); 
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_radius(cont, 10, 0);
    
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont, 5, 0);
    
    // Giorno
    forecast_days[index] = lv_label_create(cont);
    lv_obj_set_style_text_font(forecast_days[index], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(forecast_days[index], lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(forecast_days[index], "---");

    // Icona Container (Spazio vuoto dove disegneremo)
    forecast_icon_containers[index] = lv_obj_create(cont);
    lv_obj_set_size(forecast_icon_containers[index], 80, 60); // Box per l'icona
    lv_obj_set_style_bg_opa(forecast_icon_containers[index], 0, 0);
    lv_obj_set_style_border_width(forecast_icon_containers[index], 0, 0);
    // Disabilita scrolling per evitare barre scorrimento sulle icone
    lv_obj_remove_flag(forecast_icon_containers[index], LV_OBJ_FLAG_SCROLLABLE);

    // Temperatura
    forecast_temps[index] = lv_label_create(cont);
    lv_obj_set_style_text_font(forecast_temps[index], &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(forecast_temps[index], lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(forecast_temps[index], "--°");
}

void create_main_ui() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x050505), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x001030), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);

    // --- SEZIONE SUPERIORE (HEADER) ---
    lv_obj_t *top_section = lv_obj_create(scr);
    lv_obj_set_size(top_section, 800, 250);
    lv_obj_set_style_bg_opa(top_section, 0, 0); 
    lv_obj_set_style_border_width(top_section, 0, 0);
    lv_obj_align(top_section, LV_ALIGN_TOP_MID, 0, 10);
    
    // --- OROLOGIO FLEX (Stabile) ---
    lv_obj_t *cont_clock = lv_obj_create(top_section);
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

    // Data (Allineata sotto l'orologio container)
    lbl_date = lv_label_create(top_section);
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_date, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align_to(lbl_date, cont_clock, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_label_set_text(lbl_date, "Avvio...");

    // Meteo Attuale a destra
    lbl_current_temp = lv_label_create(top_section);
    lv_obj_set_style_text_font(lbl_current_temp, &lv_font_montserrat_36, 0); 
    lv_obj_set_style_text_color(lbl_current_temp, lv_color_hex(0xF1C40F), 0); 
    lv_obj_align(lbl_current_temp, LV_ALIGN_TOP_RIGHT, -30, 20);
    lv_label_set_text(lbl_current_temp, "--°");

    lbl_current_desc = lv_label_create(top_section);
    lv_obj_set_style_text_font(lbl_current_desc, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_current_desc, lv_color_hex(0xFFFFFF), 0);
    
    // Scroll circolare per testo lungo
    lv_obj_set_width(lbl_current_desc, 300); 
    lv_obj_set_style_text_align(lbl_current_desc, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_long_mode(lbl_current_desc, LV_LABEL_LONG_SCROLL_CIRCULAR);
    
    lv_obj_align(lbl_current_desc, LV_ALIGN_TOP_RIGHT, -30, 70);
    lv_label_set_text(lbl_current_desc, "---");

    // --- SEZIONE INFERIORE (Previsioni) ---
    lv_obj_t *bot_section = lv_obj_create(scr);
    lv_obj_set_size(bot_section, 780, 200);
    lv_obj_set_style_bg_opa(bot_section, 0, 0);
    lv_obj_set_style_border_width(bot_section, 0, 0);
    lv_obj_align(bot_section, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    lv_obj_set_layout(bot_section, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bot_section, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bot_section, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(bot_section, 10, 0);

    for(int i=0; i<5; i++) {
        create_forecast_box(bot_section, i);
    }

    // IP
    lbl_ip = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_ip, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_opa(lbl_ip, LV_OPA_50, 0);
    lv_obj_align(lbl_ip, LV_ALIGN_BOTTOM_LEFT, 5, -2);
    lv_label_set_text(lbl_ip, "...");
}

void update_current_weather(String temp, String desc, String iconCode) {
    lv_label_set_text_fmt(lbl_current_temp, "%s°C", temp.c_str());
    lv_label_set_text(lbl_current_desc, desc.c_str());
}

void update_forecast_item(int index, String day, String temp, String iconCode) {
    if(index < 0 || index >= 5) return;
    
    lv_label_set_text(forecast_days[index], day.c_str());
    lv_label_set_text_fmt(forecast_temps[index], "%s°", temp.c_str());
    
    render_weather_icon(forecast_icon_containers[index], iconCode);
}

void update_ui() {
    if (!ui_lbl_hour) return; 

    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);

    if (timeinfo->tm_year > 120) {
        char bufHour[4];
        char bufMin[4];
        strftime(bufHour, sizeof(bufHour), "%H", timeinfo);
        strftime(bufMin, sizeof(bufMin), "%M", timeinfo);
        
        if(strcmp(lv_label_get_text(ui_lbl_hour), bufHour) != 0) lv_label_set_text(ui_lbl_hour, bufHour);
        if(strcmp(lv_label_get_text(ui_lbl_min), bufMin) != 0) lv_label_set_text(ui_lbl_min, bufMin);

        // Lampeggio opacità
        if (timeinfo->tm_sec % 2 == 0) lv_obj_set_style_text_opa(ui_lbl_dots, LV_OPA_COVER, 0);
        else lv_obj_set_style_text_opa(ui_lbl_dots, LV_OPA_0, 0);

        char dateBuf[32];
        strftime(dateBuf, sizeof(dateBuf), "%a %d %B", timeinfo);
        dateBuf[0] = toupper(dateBuf[0]);
        lv_label_set_text(lbl_date, dateBuf);
    } else {
        static int dot = 0;
        dot = (dot + 1) % 4;
        if(dot == 0) lv_label_set_text(lbl_date, "Sync");
        if(dot == 1) lv_label_set_text(lbl_date, "Sync.");
        if(dot == 2) lv_label_set_text(lbl_date, "Sync..");
        if(dot == 3) lv_label_set_text(lbl_date, "Sync...");
    }

    if (WiFi.status() == WL_CONNECTED) {
         String ip = WiFi.localIP().toString();
         lv_label_set_text_fmt(lbl_ip, "IP: %s", ip.c_str());
    }
}