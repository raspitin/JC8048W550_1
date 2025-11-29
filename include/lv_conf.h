#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

#define LV_MEM_SIZE (128 * 1024U)  // Aumentato da 96KB a 128KB

// CRITICO: Refresh rate sincronizzato con display (30 FPS = ~33ms)
#define LV_DEF_REFR_PERIOD  33

#define LV_DPI_DEF 130

// Ottimizzazioni per ridurre carico DMA
#define LV_DISPLAY_RENDER_MODE LV_DISPLAY_RENDER_MODE_PARTIAL  // Rendering parziale
#define LV_USE_PERF_MONITOR 0  // Disabilita monitor performance (riduce overhead)

// Font Abilitati
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_36 1

#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_USE_LOG 0

// Widget
#define LV_USE_LABEL 1
#define LV_USE_ARC 1
#define LV_USE_QRCODE 1

// Cache ottimizzazioni
#define LV_CACHE_DEF_SIZE 0  // Disabilita cache se non necessaria

#endif