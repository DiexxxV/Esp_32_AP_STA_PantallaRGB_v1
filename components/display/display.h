#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ================= INIT / BASICOS =================
void display_init(void);
void display_fill(uint8_t r, uint8_t g, uint8_t b);
void display_clear(void);

// ================= TEXTO =================
void display_draw_text(int x, int y, const char *text, uint8_t r, uint8_t g, uint8_t b);

// ================= IMAGEN =================
void display_draw_image(const uint16_t *img);

// ================= LOGO RGB CONTROL =================
void display_set_logo_color(uint8_t r, uint8_t g, uint8_t b);
void display_draw_logo_colored(const uint16_t *img);

// ================= IMPORTANTE =================
extern const uint16_t logo[4096];

#ifdef __cplusplus
}
#endif

#endif