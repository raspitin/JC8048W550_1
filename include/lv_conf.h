#ifndef LV_CONF_H
#define LV_CONF_H

// --- CONFIGURAZIONE MEMORIA ---
// Aumentiamo drasticamente la memoria heap di LVGL.
// Avendo la PSRAM, possiamo spingerci oltre i 96KB.
// 256KB è un ottimo valore per gestire molte schermate e widget.
#define LV_MEM_SIZE (256 * 1024U) 

// --- RENDERING ---
#define LV_COLOR_DEPTH 16
#define LV_DEF_REFR_PERIOD 33  // 30 FPS (Standard per questi display)
#define LV_DPI_DEF 130

// --- FUNZIONI STANDARD ---
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

// --- FONT ABILITATI ---
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_36 1

// --- WIDGET & FEATURES ---
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_USE_LOG 0
#define LV_USE_LABEL 1
#define LV_USE_ARC 1
#define LV_USE_QRCODE 1 
#define LV_USE_ROLLER 1
#define LV_USE_MSGBOX 1
#define LV_USE_TABVIEW 1

// Ottimizzazioni extra
#define LV_USE_ASSERT_NULL 1   // Aiuta a catturare puntatori nulli
#define LV_USE_ASSERT_MEM_INTEGRITY 0 // Disabilita in produzione per velocità

#endif