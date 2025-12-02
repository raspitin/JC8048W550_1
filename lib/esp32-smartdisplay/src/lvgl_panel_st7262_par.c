#ifdef DISPLAY_ST7262_PAR

#include <esp32_smartdisplay.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include <esp32_smartdisplay_dma_helpers.h>
#include <esp_heap_caps.h>

// Funzione fondamentale per la nitidezza su ESP32-S3
extern void Cache_WriteBack_Addr(uint32_t addr, uint32_t size);

bool direct_io_frame_trans_done(esp_lcd_panel_handle_t panel, esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    return false;
}

void direct_io_lv_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    // 1. SCARICA LA CACHE (Fix per dati mancanti/neri)
    size_t size = (lv_area_get_width(area) * lv_area_get_height(area) * sizeof(lv_color_t));
    Cache_WriteBack_Addr((uint32_t)px_map, size);

    // 2. DISEGNA (Scambia il buffer)
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)display->user_data;
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map));
    
    // 3. NOTIFICA LVGL
    lv_display_flush_ready(display);
}

lv_display_t *lvgl_lcd_init()
{
    lv_display_t *display = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    log_v("display:0x%08x", display);
    
    // Calcolo buffer per SCHERMO INTERO (Direct Mode)
    uint32_t px_size = sizeof(lv_color_t);
    uint32_t drawBufferSize = px_size * LVGL_BUFFER_PIXELS;

    // --- ALLOCAZIONE IN PSRAM ALLINEATA (Fix Tearing & Glitch) ---
    void *drawBuffer1 = heap_caps_aligned_alloc(64, drawBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    void *drawBuffer2 = heap_caps_aligned_alloc(64, drawBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!drawBuffer1 || !drawBuffer2) log_e("CRITICO: Fallita allocazione Buffer in PSRAM!");

    // Pulizia iniziale (Nero)
    if (drawBuffer1) memset(drawBuffer1, 0, drawBufferSize);
    if (drawBuffer2) memset(drawBuffer2, 0, drawBufferSize);

    // Configurazione: DIRECT MODE (Doppio Buffer Reale)
    lv_display_set_buffers(display, drawBuffer1, drawBuffer2, drawBufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    // --- CONFIGURAZIONE DRIVER ---
    const esp_lcd_rgb_panel_config_t rgb_panel_config = {
        .clk_src = ST7262_PANEL_CONFIG_CLK_SRC,
        .timings = {
            .pclk_hz = ST7262_PANEL_CONFIG_TIMINGS_PCLK_HZ,
            .h_res = ST7262_PANEL_CONFIG_TIMINGS_H_RES,
            .v_res = ST7262_PANEL_CONFIG_TIMINGS_V_RES,
            // TIMING AGGIUSTATI PER CENTRARE L'IMMAGINE
            .hsync_pulse_width = 4,
            .hsync_back_porch = 8,   // Ridotto per spostare a sinistra
            .hsync_front_porch = 82, 
            .vsync_pulse_width = 4,
            .vsync_back_porch = 12,
            .vsync_front_porch = 12,
            .flags = {
                .hsync_idle_low = ST7262_PANEL_CONFIG_TIMINGS_FLAGS_HSYNC_IDLE_LOW,
                .vsync_idle_low = ST7262_PANEL_CONFIG_TIMINGS_FLAGS_VSYNC_IDLE_LOW,
                .de_idle_high = ST7262_PANEL_CONFIG_TIMINGS_FLAGS_DE_IDLE_HIGH,
                .pclk_active_neg = ST7262_PANEL_CONFIG_TIMINGS_FLAGS_PCLK_ACTIVE_NEG,
                .pclk_idle_high = ST7262_PANEL_CONFIG_TIMINGS_FLAGS_PCLK_IDLE_HIGH
            }
        },
        .data_width = ST7262_PANEL_CONFIG_DATA_WIDTH,
        .sram_trans_align = 64,
        .psram_trans_align = 64,
        .hsync_gpio_num = ST7262_PANEL_CONFIG_HSYNC,
        .vsync_gpio_num = ST7262_PANEL_CONFIG_VSYNC,
        .de_gpio_num = ST7262_PANEL_CONFIG_DE,
        .pclk_gpio_num = ST7262_PANEL_CONFIG_PCLK,
        .data_gpio_nums = {
            ST7262_PANEL_CONFIG_DATA_R0, ST7262_PANEL_CONFIG_DATA_R1, ST7262_PANEL_CONFIG_DATA_R2, ST7262_PANEL_CONFIG_DATA_R3, ST7262_PANEL_CONFIG_DATA_R4, 
            ST7262_PANEL_CONFIG_DATA_G0, ST7262_PANEL_CONFIG_DATA_G1, ST7262_PANEL_CONFIG_DATA_G2, ST7262_PANEL_CONFIG_DATA_G3, ST7262_PANEL_CONFIG_DATA_G4, ST7262_PANEL_CONFIG_DATA_G5, 
            ST7262_PANEL_CONFIG_DATA_B0, ST7262_PANEL_CONFIG_DATA_B1, ST7262_PANEL_CONFIG_DATA_B2, ST7262_PANEL_CONFIG_DATA_B3, ST7262_PANEL_CONFIG_DATA_B4
        },
        .disp_gpio_num = ST7262_PANEL_CONFIG_DISP,
        .on_frame_trans_done = direct_io_frame_trans_done,
        .user_ctx = display,
        // fb_in_psram = true serve per evitare crash di memoria interna
        .flags = {
            .disp_active_low = ST7262_PANEL_CONFIG_FLAGS_DISP_ACTIVE_LOW, 
            .relax_on_idle = ST7262_PANEL_CONFIG_FLAGS_RELAX_ON_IDLE, 
            .fb_in_psram = true 
        }
    };

    esp_lcd_panel_handle_t panel_handle;
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&rgb_panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    
    display->user_data = panel_handle;
    display->flush_cb = direct_io_lv_flush;

    return display;
}

#endif