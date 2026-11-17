#include "mock_waveshare.h"

namespace display::test {
MockWaveshare *waveshare = nullptr;
}

extern "C" {
UBYTE DEV_ModuleInit(void) { return display::test::waveshare->moduleInit(); }
void DEV_ModuleExit(void) { display::test::waveshare->moduleExit(); }
void DEV_SetBacklight(UWORD value) { display::test::waveshare->backlight(value); }
void LCD_2IN_Init(void) { display::test::waveshare->lcdInit(); }
void LCD_2IN_Clear(UWORD color) { display::test::waveshare->lcdClear(color); }
void LCD_2IN_Display(UBYTE *image) { display::test::waveshare->display(image); }
void Paint_NewImage(UWORD *image, UWORD width, UWORD height, UWORD rotation,
                    UWORD color, UWORD depth) {
  display::test::waveshare->newImage(image, width, height, rotation, color, depth);
}
void Paint_SetRotate(UWORD rotation) { display::test::waveshare->rotate(rotation); }
void Paint_SelectImage(UWORD *image) { display::test::waveshare->selectImage(image); }
void Paint_Clear(UWORD color) { display::test::waveshare->clear(color); }
void Paint_DrawString_EN(UWORD x, UWORD y, const char *text, sFONT *font,
                         UWORD foreground, UWORD background) {
  display::test::waveshare->drawText(x, y, text, font, foreground, background);
}
}
