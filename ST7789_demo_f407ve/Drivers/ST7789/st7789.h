#ifndef __ST7789_H
#define __ST7789_H

#include <images.h>
#include "ugui.h"
#include "main.h"

/* choose a Hardware SPI port to use. */
#define ST7789_SPI_PORT     hspi1
extern SPI_HandleTypeDef    ST7789_SPI_PORT;

/* Pin connections */
#define ST7789_RST_PORT     LCD_RST_GPIO_Port
#define ST7789_RST_PIN      LCD_RST_Pin

#define ST7789_DC_PORT      LCD_DC_GPIO_Port
#define ST7789_DC_PIN       LCD_DC_Pin

/* Disable these two lines if your display has no CS pin */
#define ST7789_CS_PORT      LCD_CS_GPIO_Port
#define ST7789_CS_PIN       LCD_CS_Pin

/* Enable if you need backlight control */
//#define ST7789_BLK_PORT     LCD_BLK_GPIO_Port
//#define ST7789_BLK_PIN      LCD_BLK_GPIO_Port

// Disable to don't use DMA
#define USE_DMA

/**
 * Comment one to use another one.
 * two parameters can be chosen
 * 135x240(0.96 inch) and 240x240(1.3inch) 
 * X_SHIFT&Y_SHIFT are used to correct different display's resolution
 */

/* Choose a type you are using */
#define USING_135X240
//#define USING_240X240

/* Choose a display rotation you want to use: (0-3) */
//#define ST7789_ROTATION 0
//#define ST7789_ROTATION 1
//#define ST7789_ROTATION 2       //  use Normally on 240x240
#define ST7789_ROTATION 3

#ifdef USING_135X240

    #if ST7789_ROTATION == 0
        #define ST7789_WIDTH 135
        #define ST7789_HEIGHT 240
        #define X_SHIFT 53
        #define Y_SHIFT 40
    #endif

    #if ST7789_ROTATION == 1
        #define ST7789_WIDTH 240
        #define ST7789_HEIGHT 135
        #define X_SHIFT 40
        #define Y_SHIFT 52
    #endif

    #if ST7789_ROTATION == 2
        #define ST7789_WIDTH 135
        #define ST7789_HEIGHT 240
        #define X_SHIFT 52
        #define Y_SHIFT 40
    #endif

    #if ST7789_ROTATION == 3
        #define ST7789_WIDTH 240
        #define ST7789_HEIGHT 135
        #define X_SHIFT 40
        #define Y_SHIFT 53
    #endif

#endif

#ifdef USING_240X240

    #define ST7789_WIDTH 240
    #define ST7789_HEIGHT 240

    #if ST7789_ROTATION == 0
      #define X_SHIFT 0
      #define Y_SHIFT 80
    #elif ST7789_ROTATION == 1
      #define X_SHIFT 80
      #define Y_SHIFT 0
    #elif ST7789_ROTATION == 2
      #define X_SHIFT 0
      #define Y_SHIFT 0
    #elif ST7789_ROTATION == 3
      #define X_SHIFT 0
      #define Y_SHIFT 0
    #endif

#endif

/**
 *Color of pen
 *If you want to use another color, you can choose one in RGB565 format.
 */


/* Control Registers and constant codes */
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID   0x04
#define ST7789_RDDST   0x09

#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_PTLON   0x12
#define ST7789_NORON   0x13

#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define ST7789_PTLAR   0x30
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36

/** 
 * Memory Data Access Control Register (0x36H)
 * MAP:     D7  D6  D5  D4  D3  D2  D1  D0 
 * param:   MY  MX  MV  ML  RGB MH  -   -
 * 
 */ 

/* Page Address Order ('0': Top to Bottom, '1': the opposite) */
#define ST7789_MADCTL_MY  0x80  
/* Column Address Order ('0': Left to Right, '1': the opposite) */
#define ST7789_MADCTL_MX  0x40  
/* Page/Column Order ('0' = Normal Mode, '1' = Reverse Mode) */
#define ST7789_MADCTL_MV  0x20  
/* Line Address Order ('0' = LCD Refresh Top to Bottom, '1' = the opposite) */
#define ST7789_MADCTL_ML  0x10
/* RGB/BGR Order ('0' = RGB, '1' = BGR) */
#define ST7789_MADCTL_RGB 0x00

#define ST7789_RDID1   0xDA
#define ST7789_RDID2   0xDB
#define ST7789_RDID3   0xDC
#define ST7789_RDID4   0xDD

/* Advanced options */
/**
 * Caution: Do not operate these settings
 * You know what you are doing 
 */

#define ST7789_COLOR_MODE_16bit 0x55    //  RGB565 (16bit)
#define ST7789_COLOR_MODE_18bit 0x66    //  RGB666 (18bit)

/* Basic operations */
#define ST7789_RST_Clr()    ST7789_RST_PORT->BSRR=ST7789_RST_PIN<<16
#define ST7789_RST_Set()    ST7789_RST_PORT->BSRR=ST7789_RST_PIN

#define ST7789_DC_Clr()     ST7789_DC_PORT->BSRR=ST7789_DC_PIN<<16
#define ST7789_DC_Set()     ST7789_DC_PORT->BSRR=ST7789_DC_PIN

#if defined ST7789_CS_PORT && defined ST7789_CS_PIN
#define ST7789_Select()     ST7789_CS_PORT->BSRR=ST7789_CS_PIN<<16
#define ST7789_UnSelect()   ST7789_CS_PORT->BSRR=ST7789_CS_PIN
#else
#define ST7789_Select()
#define ST7789_UnSelect()
#endif

#if defined ST7789_BLK_PORT && defined ST7789_BLK_PIN
#define ST7789_BLK_Clr()     ST7789_BLK_PORT->BSRR=ST7789_BLK_PIN<<16
#define ST7789_BLK_Set()     ST7789_BLK_PORT->BSRR=ST7789_BLK_PIN
#else
#define ST7789_BLK_Clr()
#define ST7789_BLK_Set()
#endif
#define ABS(x) ((x) > 0 ? (x) : -(x))
/* Basic functions. */
void ST7789_Init(void);
void ST7789_SetRotation(uint8_t m);
void ST7789_DrawPixel(int16_t x, int16_t y, uint16_t color);
int8_t  ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color);

/* Graphical functions. */
int8_t  ST7789_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, void * data);
void ST7789_InvertColors(uint8_t invert);

/* Text functions. */
void ST7789_PutChar(uint16_t x, uint16_t y, char ch, UG_FONT* font, uint16_t color, uint16_t bgcolor);
void ST7789_PutStr(uint16_t x, uint16_t y,  char *str, UG_FONT* font, uint16_t color, uint16_t bgcolor);

/* Extended Graphical functions. */
/* Command functions */
void ST7789_TearEffect(uint8_t tear);

/* Simple test function. */
void ST7789_Test(void);

#if !defined(USING_240X240)
  #if !defined(USING_135X240)
      #error  You should at least choose one display resolution!
  #endif
#endif

#if !defined(USING_135X240)
  #if !defined(USING_240X240)
      #error  You should at least choose one display resolution!
  #endif
#endif


#ifndef ST7789_ROTATION
    #error You should at least choose a display rotation!
#endif

#endif
