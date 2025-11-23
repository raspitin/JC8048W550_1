#ifndef UI_H
#define UI_H
#include <Arduino.h>

void create_nest_ui();
void update_ui();
void ui_show_setup_screen(const char* ssid, const char* pass);
void ui_show_connecting();
void ui_hide_setup_screen();
void ui_log(String text);

#endif