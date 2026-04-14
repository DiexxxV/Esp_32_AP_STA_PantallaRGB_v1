#include "display.h"
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"

#define PANEL_RES_X 64
#define PANEL_RES_Y 64
#define PANEL_CHAIN 1

// ===== PINES =====
#define R1 25
#define G1 26
#define B1 27
#define R2 21
#define G2 22
#define B2 13

#define A 12
#define B 19
#define C 17
#define D 18
#define E 5

#define CLK 15
#define LAT 32
#define OE 33

static MatrixPanel_I2S_DMA *dma_display = nullptr;

// ================= LOGO =================
const uint16_t logo[4096] = {
#include "logo_data.inc"
};

// 🔥 COLOR GLOBAL DEL LOGO
static uint8_t logo_r = 255;
static uint8_t logo_g = 255;
static uint8_t logo_b = 255;

// ================= INIT =================
void display_init(void)
{
    HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);

    mxconfig.gpio.r1 = R1;
    mxconfig.gpio.g1 = G1;
    mxconfig.gpio.b1 = B1;

    mxconfig.gpio.r2 = R2;
    mxconfig.gpio.g2 = G2;
    mxconfig.gpio.b2 = B2;

    mxconfig.gpio.a = A;
    mxconfig.gpio.b = B;
    mxconfig.gpio.c = C;
    mxconfig.gpio.d = D;
    mxconfig.gpio.e = E;

    mxconfig.gpio.clk = CLK;
    mxconfig.gpio.lat = LAT;
    mxconfig.gpio.oe  = OE;

    mxconfig.clkphase = false;

    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    dma_display->begin();

    dma_display->clearScreen();
    dma_display->flipDMABuffer();

    dma_display->setBrightness8(40);
    dma_display->clearScreen();
}

// ================= BASICOS =================
void display_fill(uint8_t r, uint8_t g, uint8_t b)
{
    if (!dma_display) return;

    dma_display->fillScreen(dma_display->color565(r, g, b));
}

void display_clear(void)
{
    if (!dma_display) return;
    dma_display->clearScreen();
}

// ================= 🔥 LOGO COLOR =================
void display_set_logo_color(uint8_t r, uint8_t g, uint8_t b)
{
    logo_r = r;
    logo_g = g;
    logo_b = b;
}

// convierte pixel blanco → color elegido
static inline uint16_t logo_color_map(uint16_t px)
{
    // negro = transparente
    if (px == 0x0000) return 0;

    return dma_display->color565(logo_r, logo_g, logo_b);
}

// ================= DIBUJAR LOGO =================
void display_draw_image(const uint16_t *img)
{
    if (!dma_display || !img) return;

    for (int y = 0; y < PANEL_RES_Y; y++)
    {
        for (int x = 0; x < PANEL_RES_X; x++)
        {
            dma_display->drawPixel(x, y, img[y * PANEL_RES_X + x]);
        }
    }
}

// 🔥 LOGO CON COLOR DINÁMICO (USADO DESDE WEB)
void display_draw_logo_colored(const uint16_t *img)
{
    if (!dma_display || !img) return;

    for (int y = 0; y < PANEL_RES_Y; y++)
    {
        for (int x = 0; x < PANEL_RES_X; x++)
        {
            uint16_t px = img[y * PANEL_RES_X + x];

            if (px != 0x0000)
            {
                dma_display->drawPixel(x, y, logo_color_map(px));
            }
        }
    }
}

// ================= FUENTE =================
static const uint8_t font5x7[][5] = {
    {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22}, {0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00}, {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x04,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E}, {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F}, {0x1F,0x20,0x40,0x20,0x1F},
    {0x7F,0x20,0x18,0x20,0x7F}, {0x63,0x14,0x08,0x14,0x63},
    {0x03,0x04,0x78,0x04,0x03}, {0x61,0x51,0x49,0x45,0x43}
};

// ================= TEXTO =================
static void draw_char(int x, int y, char c, uint16_t color)
{
    if (!dma_display) return;

    if (c >= 'a' && c <= 'z') c -= 32;
    if (c < 'A' || c > 'Z') return;

    const uint8_t *bitmap = font5x7[c - 'A'];

    for (int col = 0; col < 5; col++)
    {
        for (int row = 0; row < 7; row++)
        {
            if (bitmap[col] & (1 << row))
            {
                dma_display->drawPixel(x + col, y + row, color);
            }
        }
    }
}

void display_draw_text(int x, int y, const char *text,
                       uint8_t r, uint8_t g, uint8_t b)
{
    if (!dma_display) return;

    uint16_t color = dma_display->color565(r, g, b);

    while (*text)
    {
        draw_char(x, y, *text, color);
        x += 6;
        text++;
    }
}