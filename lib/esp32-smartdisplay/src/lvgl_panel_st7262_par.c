#ifdef DISPLAY_ST7262_PAR

#include <esp32_smartdisplay.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include <esp32_smartdisplay_dma_helpers.h>
#include <esp_heap_caps.h>

extern void Cache_WriteBack_Addr(uint32_t addr, uint32_t size);

bool direct_io_frame_trans_done(esp_lcd_panel_handle_t panel, esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    return false;
}

void direct_io_lv_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    size_t size = (lv_area_get_width(area) * lv_area_get_height(area) * sizeof(lv_color_t));
    Cache_WriteBack_Addr((uint32_t)px_map, size);

    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)display->user_data;
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map));
    
    lv_display_flush_ready(display);
}

lv_display_t *lvgl_lcd_init()
{
    lv_display_t *display = lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    uint32_t px_size = sizeof(lv_color_t);
    uint32_t drawBufferSize = px_size * LVGL_BUFFER_PIXELS;

    // Allocazione buffer manuale in PSRAM (Allineata)
    void *drawBuffer1 = heap_caps_aligned_alloc(64, drawBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    void *drawBuffer2 = heap_caps_aligned_alloc(64, drawBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (drawBuffer1) memset(drawBuffer1, 0, drawBufferSize);
    if (drawBuffer2) memset(drawBuffer2, 0, drawBufferSize);

    lv_display_set_buffers(display, drawBuffer1, drawBuffer2, drawBufferSize, LV_DISPLAY_RENDER_MODE_DIRECT);
    
    const esp_lcd_rgb_panel_config_t rgb_panel_config = {
        .clk_src = ST7262_PANEL_CONFIG_CLK_SRC,
        .timings = {
            .pclk_hz = ST7262_PANEL_CONFIG_TIMINGS_PCLK_HZ,
            .h_res = ST7262_PANEL_CONFIG_TIMINGS_H_RES,
            .v_res = ST7262_PANEL_CONFIG_TIMINGS_V_RES,
            // --- FIX LAYOUT SPOSTATO A DESTRA ---
            // Riduciamo il Back Porch (era 45) per iniziare a disegnare prima (più a sinistra)
            // Aumentiamo il Front Porch (era 45) per mantenere il tempo totale di riga simile
            .hsync_pulse_width = 4,
            .hsync_back_porch = 8,   // <--- MODIFICATO (Era 45): Sposta immagine a sinistra
            .hsync_front_porch = 82, // <--- MODIFICATO (Era 45): Bilancia il tempo totale
            
            .vsync_pulse_width = 4,
            .vsync_back_porch = 12,  // Leggermente ridotto per sicurezza verticale
            .vsync_front_porch = 12,
            
            .flags = {
                .hsync_idle_low = 0,
                .vsync_idle_low = 0,
                .de_idle_high = 0,
                .pclk_active_neg = 0, // A volte invertire questo aiuta con il rumore, ma per ora lascia 0
                .pclk_idle_high = 0
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