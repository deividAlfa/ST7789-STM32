#include "st7789.h"
#include "stdio.h"
#include "stdint.h"

#define mode_16bit    1
#define mode_8bit     0
/*
 * @brief Sets SPI interface word size (0=8bit, 1=16 bit)
 * @param none
 * @return none
 */
static void setSPI_Size(uint8_t is16Bit){
  if(is16Bit){
    ST7789_SPI_PORT.Init.DataSize = SPI_DATASIZE_16BIT;
  }
  else{
    ST7789_SPI_PORT.Init.DataSize = SPI_DATASIZE_8BIT;
  }
  HAL_SPI_Init(&ST7789_SPI_PORT);
}

#ifdef USE_DMA
#define DMA_min_Sz    16
#define mem_increase  1
#define mem_fixed     0
/**
 * @brief Configures DMA/ SPI interface
 * @param memInc Enable/disable memory address increase
 * @param mode16 Enable/disable 16 bit mode (disabled = 8 bit)
 * @return none
 */
static void setDMAMemMode(uint8_t memInc, uint8_t is16Bit){

  setSPI_Size(is16Bit);

  __HAL_DMA_DISABLE(ST7789_SPI_PORT.hdmatx);
  if(memInc){
    ST7789_SPI_PORT.hdmatx->Init.MemInc = DMA_MINC_ENABLE;
  }
  else{
    ST7789_SPI_PORT.hdmatx->Init.MemInc = DMA_MINC_DISABLE;
  }
  if(is16Bit){
    ST7789_SPI_PORT.hdmatx->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    ST7789_SPI_PORT.hdmatx->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  }
  else{
    ST7789_SPI_PORT.hdmatx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    ST7789_SPI_PORT.hdmatx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  }
  HAL_DMA_Init(ST7789_SPI_PORT.hdmatx);
}
#endif

/**
 * @brief Write command to ST7789 controller
 * @param cmd -> command to write
 * @return none
 */
static void ST7789_WriteCommand(uint8_t cmd)
{
  ST7789_Select();
  ST7789_DC_Clr();
  HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
  ST7789_UnSelect();
}

/**
 * @brief Write data to ST7789 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static void ST7789_WriteData(uint8_t *buff, size_t buff_size)
{
  ST7789_Select();
  ST7789_DC_Set();

  // split data in small chunks because HAL can't send more than 64K at once

  while (buff_size > 0) {
    uint16_t chunk_size = buff_size > 65535 ? 65535 : buff_size;
    #ifdef USE_DMA
    if(DMA_min_Sz<=buff_size){
      HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, buff, chunk_size);
      while(ST7789_SPI_PORT.hdmatx->State!=HAL_DMA_STATE_READY){
        asm("nop");                                              // Fix for current STM32F1 libraries, HAL_DMA_StateTypeDef is not being declared as volatile so optimizations will break this check
      }
    }
    else{
      HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
    }
    #else
    HAL_SPI_Transmit(&ST7789_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
    #endif
    buff += chunk_size;
    buff_size -= chunk_size;
  }

  ST7789_UnSelect();
}
/**
 * @brief Write data to ST7789 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static void ST7789_WriteSmallData(uint8_t data)
{
  ST7789_Select();
  ST7789_DC_Set();
  HAL_SPI_Transmit(&ST7789_SPI_PORT, &data, sizeof(data), HAL_MAX_DELAY);
  ST7789_UnSelect();
}

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in st7789.h)
 * @return none
 */
void ST7789_SetRotation(uint8_t m)
{
  ST7789_WriteCommand(ST7789_MADCTL); // MADCTL
  switch (m) {
  case 0:
    ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
    break;
  case 1:
    ST7789_WriteSmallData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
    break;
  case 2:
    ST7789_WriteSmallData(ST7789_MADCTL_RGB);
    break;
  case 3:
    ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
    break;
  default:
    break;
  }
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
  uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;

  /* Column Address set */
  ST7789_WriteCommand(ST7789_CASET);
  {
    uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
    ST7789_WriteData(data, sizeof(data));
  }

  /* Row Address set */
  ST7789_WriteCommand(ST7789_RASET);
  {
    uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
    ST7789_WriteData(data, sizeof(data));
  }
  /* Write to RAM */
  ST7789_WriteCommand(ST7789_RAMWR);
}

/**
 * @brief Initialize ST7789 controller
 * @param none
 * @return none
 */
void ST7789_Init(void)
{
  setSPI_Size(mode_8bit);
  ST7789_UnSelect();
  ST7789_RST_Clr();
  HAL_Delay(1);
  ST7789_RST_Set();
  HAL_Delay(120);

  ST7789_WriteCommand(ST7789_COLMOD);   //  Set color mode
  ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);
  ST7789_WriteCommand(0xB2);        //  Porch control
  {
    uint8_t data[] = {0x01, 0x01, 0x00, 0x11, 0x11};            // Minimum porch (7% faster screen refresh rate)   *** Restore normal value if having problems ***
    //uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};          // Standard porch
    ST7789_WriteData(data, sizeof(data));
  }
  ST7789_SetRotation(ST7789_ROTATION);  //  MADCTL (Display Rotation)

  /* Internal LCD Voltage generator settings */
  ST7789_WriteCommand(0XB7);        //  Gate Control
  ST7789_WriteSmallData(0x35);      //  Default value
  ST7789_WriteCommand(0xBB);        //  VCOM setting
  ST7789_WriteSmallData(0x19);      //  0.725v (default 0.75v for 0x20)
  ST7789_WriteCommand(0xC0);        //  LCMCTRL
  ST7789_WriteSmallData (0x2C);     //  Default value
  ST7789_WriteCommand (0xC2);       //  VDV and VRH command Enable
  ST7789_WriteSmallData (0x01);     //  Default value
  ST7789_WriteCommand (0xC3);       //  VRH set
  ST7789_WriteSmallData (0x12);     //  +-4.45v (default +-4.1v for 0x0B)
  ST7789_WriteCommand (0xC4);       //  VDV set
  ST7789_WriteSmallData (0x20);     //  Default value
  ST7789_WriteCommand (0xC6);       //  Frame rate control in normal mode
  ST7789_WriteSmallData (0x01);     //  Max refresh rate (111Hz).           *** Restore normal value if having problems ***
  //ST7789_WriteSmallData (0x0F);     //  Default refresh rate (60Hz)
  ST7789_WriteCommand (0xD0);       //  Power control
  ST7789_WriteSmallData (0xA4);     //  Default value
  ST7789_WriteSmallData (0xA1);     //  Default value a1
  /**************** Division line ****************/

  ST7789_WriteCommand(0xE0);
  {
    uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
    ST7789_WriteData(data, sizeof(data));
  }

  ST7789_WriteCommand(0xE1);
  {
    uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
    ST7789_WriteData(data, sizeof(data));
  }
  ST7789_WriteCommand (ST7789_INVON);   //  Inversion ON
  ST7789_WriteCommand (ST7789_SLPOUT);  //  Out of sleep mode
  ST7789_WriteCommand (ST7789_NORON);   //  Normal Display on

  ST7789_Fill_Color(GREEN);             //  Fill
  ST7789_WriteCommand (ST7789_DISPON);  //  Main screen turned on

  HAL_Delay(500);
}



/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
  if ((x < 0) || (x >= ST7789_WIDTH) ||
     (y < 0) || (y >= ST7789_HEIGHT)) return;

  uint8_t data[2] = {color >> 8, color & 0xFF};

  ST7789_SetAddressWindow(x, y, x, y);

  ST7789_DC_Set();
  ST7789_Select();

  HAL_SPI_Transmit(&ST7789_SPI_PORT, data, sizeof(data), HAL_MAX_DELAY);
  ST7789_UnSelect();
}

/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
  uint16_t pixels = (xEnd-xSta+1)*(yEnd-ySta+1);
  if ((xEnd < 0) || (xEnd >= ST7789_WIDTH) ||
     (yEnd < 0) || (yEnd >= ST7789_HEIGHT)) return;

  ST7789_SetAddressWindow(xSta, ySta, xEnd, yEnd);

  #ifdef USE_DMA
  if(DMA_min_Sz<=pixels){
    setDMAMemMode(mem_fixed, mode_16bit);                                             // Set SPI and DMA to 16 bit, enable memory increase
    ST7789_WriteData((uint8_t*)&color, pixels);
  }
  else{
  #endif
    setSPI_Size(mode_16bit);                                                          // Set SPI to 16 bit
    uint16_t fill[64];                                                                // Use a 64 pixel (128Byte) buffer for faster filling
    uint8_t blockSz;

    if(pixels<64){                                                                    // Adjust block size
      blockSz=pixels;
    }
    else{
      blockSz=64;
    }
    for(uint8_t t=0;t<blockSz;t++){                                                   // Fill the buffer with the color
      fill[t]=color;
    }
    while(pixels>=blockSz){                                                           // Send 64 pixel blocks
      ST7789_WriteData((uint8_t*)&fill, blockSz);
      pixels-=blockSz;
    }
    if(pixels){                                                                       // Send remaining pixels
      ST7789_WriteData((uint8_t*)&fill, pixels);
    }
  #ifdef USE_DMA
  }
  #endif
  setSPI_Size(mode_8bit);                                                           // Set SPI to 8 bit
}


/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
  if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT))
    return;
  if ((x + w - 1) >= ST7789_WIDTH)
    return;
  if ((y + h - 1) >= ST7789_HEIGHT)
    return;

  ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);

  #ifdef USE_DMA
  setDMAMemMode(mem_increase, mode_16bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
  #else
  setSPI_Size(mode_16bit);                                                                            // Set SPI to 16 bit
  #endif
  ST7789_WriteData((uint8_t*)data, w*h);
  setSPI_Size(mode_8bit);                                                                             // Set SPI to 8 bit
  }

/**
 * @brief Draw a big Pixel at a point
 * @param x&y -> coordinate of the point
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
  if ((x <= 0) || (x > ST7789_WIDTH) ||
     (y <= 0) || (y > ST7789_HEIGHT)) return;
  ST7789_Select();
  ST7789_Fill(x - 1, y - 1, x + 1, y + 1, color);
  ST7789_UnSelect();
}


/**
 * @brief Draw horizontal line with single color
 * @param x0&x1 -> start/end position of X
 * @param y -> coordinate of the y position
 * @param color -> color of the line to Draw
 * @return none
 */

void ST7789_DrawHLine(uint16_t x0, uint16_t x1, uint16_t y, uint16_t color) {
  if(x1<x0){
    uint16_t temp = x0;
    x0=x1;
    x1=temp;
  }
  ST7789_Fill(x0,y,x1,y,color);
}

/**
 * @brief Draw vertical line with single color
 * @param x -> coordinate of the x position
 * @param y0&y1 -> start/end position of Y
 * @param color -> color of the line to Draw
 * @return none
 */
void ST7789_DrawVLine(uint16_t x, uint16_t y0, uint16_t y1, uint16_t color) {
  if(y1<y0){
    uint16_t temp = y0;
    y0=y1;
    y1=temp;
  }
  ST7789_Fill(x,y0,x,y1,color);
}

/**
 * @brief Draw a line with single color
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
void ST7789_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
  uint16_t swap;
  if(x0==x1){
    ST7789_DrawVLine(x0, y0, y1, color);
    return;
  }
  else if(y0==y1){
    ST7789_DrawHLine(x0, x1, y0, color);
    return;
  }
  uint16_t steep = ABS(y1 - y0) > ABS(x1 - x0);
  if (steep) {
    swap = x0;
    x0 = y0;
    y0 = swap;

    swap = x1;
    x1 = y1;
    y1 = swap;
  }

  if (x0 > x1) {
    swap = x0;
    x0 = x1;
    x1 = swap;

    swap = y0;
    y0 = y1;
    y1 = swap;
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = ABS(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
      ystep = 1;
  } else {
      ystep = -1;
  }

  for (; x0<=x1; x0++) {
      if (steep) {
          ST7789_DrawPixel(y0, x0, color);
      } else {
          ST7789_DrawPixel(x0, y0, color);
      }
      err -= dy;
      if (err < 0) {
          y0 += ystep;
          err += dx;
      }
  }
}

/**
 * @brief Draw a Rectangle with single color
 * @param xi&yi -> 2 coordinates of 2 top points.
 * @param color -> color of the Rectangle line
 * @return none
 */
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
  ST7789_DrawHLine(x1, x2, y1, color);
  ST7789_DrawHLine(x1, x2, y2, color);

  ST7789_DrawVLine(x1, y1, y2, color);
  ST7789_DrawVLine(x2, y1, y2, color);
}

/** 
 * @brief Draw a circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle line
 * @return  none
 */
void ST7789_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  ST7789_DrawPixel(x0, y0 + r, color);
  ST7789_DrawPixel(x0, y0 - r, color);
  ST7789_DrawPixel(x0 + r, y0, color);
  ST7789_DrawPixel(x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    ST7789_DrawPixel(x0 + x, y0 + y, color);
    ST7789_DrawPixel(x0 - x, y0 + y, color);
    ST7789_DrawPixel(x0 + x, y0 - y, color);
    ST7789_DrawPixel(x0 - x, y0 - y, color);

    ST7789_DrawPixel(x0 + y, y0 + x, color);
    ST7789_DrawPixel(x0 - y, y0 + x, color);
    ST7789_DrawPixel(x0 + y, y0 - x, color);
    ST7789_DrawPixel(x0 - y, y0 - x, color);
  }
}


/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void ST7789_InvertColors(uint8_t invert)
{
  ST7789_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
}

/** 
 * @brief Write a char
 * @param  x&y -> cursor of the start point.
 * @param ch -> char to write
 * @param font -> fontstyle of the string
 * @param color -> color of the char
 * @param bgcolor -> background color of the char
 * @return  none
 */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
  uint32_t i, b, j;

  uint8_t foreColor[2] = {color >> 8, color & 0xFF};
  uint8_t bgColor[2] = {bgcolor >> 8, bgcolor & 0xFF};

  ST7789_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);

  ST7789_Select();
  ST7789_DC_Set();

  for (i = 0; i < font.height; i++) {
    b = font.data[(ch - 32) * font.height + i];
    for (j = 0; j < font.width; j++) {
      if ((b << j) & 0x8000) {
        HAL_SPI_Transmit(&ST7789_SPI_PORT, foreColor, 2, HAL_MAX_DELAY);
      }
      else {
        HAL_SPI_Transmit(&ST7789_SPI_PORT, bgColor, 2, HAL_MAX_DELAY);
      }
    }
  }
  ST7789_UnSelect();
}

/** 
 * @brief Write a string 
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{
  while (*str) {
    if (x + font.width >= ST7789_WIDTH) {
      x = 0;
      y += font.height;
      if (y + font.height >= ST7789_HEIGHT) {
        break;
      }

      if (*str == ' ') {
        // skip spaces in the beginning of the new line
        str++;
        continue;
      }
    }
    ST7789_WriteChar(x, y, *str, font, color, bgcolor);
    x += font.width;
    str++;
  }
}


/** 
 * @brief Draw a Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the lines
 * @return  none
 */
void ST7789_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
  /* Draw lines */
  ST7789_DrawLine(x1, y1, x2, y2, color);
  ST7789_DrawLine(x2, y2, x3, y3, color);
  ST7789_DrawLine(x3, y3, x1, y1, color);
}

/** 
 * @brief Draw a filled Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the triangle
 * @return  none
 */
void ST7789_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
  int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
      yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
      curpixel = 0;

  deltax = ABS(x2 - x1);
  deltay = ABS(y2 - y1);
  x = x1;
  y = y1;

  if (x2 >= x1) {
    xinc1 = 1;
    xinc2 = 1;
  }
  else {
    xinc1 = -1;
    xinc2 = -1;
  }

  if (y2 >= y1) {
    yinc1 = 1;
    yinc2 = 1;
  }
  else {
    yinc1 = -1;
    yinc2 = -1;
  }

  if (deltax >= deltay) {
    xinc1 = 0;
    yinc2 = 0;
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax;
  }
  else {
    xinc2 = 0;
    yinc1 = 0;
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay;
  }

  for (curpixel = 0; curpixel <= numpixels; curpixel++) {
    ST7789_DrawLine(x, y, x3, y3, color);

    num += numadd;
    if (num >= den) {
      num -= den;
      x += xinc1;
      y += yinc1;
    }
    x += xinc2;
    y += yinc2;
  }
}

/** 
 * @brief Draw a Filled circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle
 * @return  none
 */
void ST7789_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
  ST7789_Select();
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  ST7789_DrawPixel(x0, y0 + r, color);
  ST7789_DrawPixel(x0, y0 - r, color);
  ST7789_DrawPixel(x0 + r, y0, color);
  ST7789_DrawPixel(x0 - r, y0, color);
  ST7789_DrawHLine(x0 - r, x0 + r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    ST7789_DrawHLine(x0 - x, x0 + x, y0 + y, color);
    ST7789_DrawHLine(x0 + x, x0 - x, y0 - y, color);
    ST7789_DrawHLine(x0 + y, x0 - y, y0 + x, color);
    ST7789_DrawHLine(x0 + y, x0 - y, y0 - x, color);
  }
}


/**
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void ST7789_TearEffect(uint8_t tear)
{
  ST7789_Select();
  ST7789_WriteCommand(tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */);
  ST7789_UnSelect();
}

static uint32_t draw_time=0;
static void printTime(void){
  char str[8];
  sprintf(str,"%lums",HAL_GetTick()-draw_time);
  ST7789_WriteString(160, 120, str, Font_11x18, WHITE, BLACK);
}
/** 
 * @brief A Simple test function for ST7789
 * @param  none
 * @return  none
 */
void ST7789_Test(void)
{
  ST7789_Fill_Color(WHITE);
  ST7789_WriteString(10, 20, "Fill Test starting", Font_11x18, RED, WHITE);
  HAL_Delay(1000);
  uint8_t r=0,g=0,b=0;
  for(r=0; r<32;r++){
    ST7789_Fill_Color((uint16_t)r<<11 | g<<5 | b);          // R++, G=0, B=0
  }
  r=31;
  for(g=0; g<64;g+=2){
    ST7789_Fill_Color((uint16_t)r<<11 | g<<5 | b);          // R=31, G++, B=0
  }
  g=63;
  for(r=28; r;r--){
    ST7789_Fill_Color((uint16_t)r<<11 | g<<5 | b);          // R--, Gmax, B=0
  }
  for(b=0; b<32;b++){
    ST7789_Fill_Color((uint16_t)r<<11 | g<<5 | b);          // R=0, Gmax, B++
  }
  b=31;
  for(g=56; g;g-=2){
    ST7789_Fill_Color((uint16_t)r<<11 | g<<5 | b);          // R=0, G--, Bmax
  }
  for(r=0; r<32;r++){
    ST7789_Fill_Color((uint16_t)r<<11 | g<<5 | b);          // R++, G=0, Bmax
  }
  r=31;
  for(g=0; g<64;g+=2){
    ST7789_Fill_Color((uint16_t)r<<11 | g<<5 | b);          // Rmax, G++, Bmax
  }


  ST7789_Fill_Color(RED);
  HAL_Delay(500);
  ST7789_Fill_Color(GREEN);
  HAL_Delay(500);
  ST7789_Fill_Color(BLUE);
  HAL_Delay(500);
  ST7789_Fill_Color(BLACK);
  HAL_Delay(500);
  draw_time=HAL_GetTick();
  ST7789_Fill_Color(WHITE);
  printTime();
  ST7789_WriteString(10, 20, "Fill Test", Font_11x18, RED, WHITE);
  HAL_Delay(2000);

  ST7789_Fill_Color(GRAY);
  draw_time=HAL_GetTick();
  ST7789_WriteString(10, 10, "Font test.", Font_11x18, GBLUE, GRAY);
  ST7789_WriteString(10, 50, "Hello Steve!", Font_11x18, RED, GRAY);
  ST7789_WriteString(10, 75, "Hello Steve!", Font_11x18, YELLOW, GRAY);
  ST7789_WriteString(10, 100, "Hello Steve!", Font_11x18, MAGENTA, GRAY);
  printTime();
  HAL_Delay(2000);

  ST7789_Fill_Color(RED);
  ST7789_WriteString(10, 10, "Rect./Line.", Font_11x18, YELLOW, RED);
  draw_time=HAL_GetTick();
  ST7789_DrawRectangle(30, 30, 100, 100, WHITE);
  printTime();
  HAL_Delay(1000);

  ST7789_Fill_Color(RED);
  ST7789_WriteString(10, 10, "Filled Rect.", Font_11x18, YELLOW, RED);
  draw_time=HAL_GetTick();
  ST7789_DrawFilledRectangle(30, 30, 50, 50, WHITE);
  printTime();
  HAL_Delay(1000);


  ST7789_Fill_Color(RED);
  ST7789_WriteString(10, 10, "Circle.", Font_11x18, YELLOW, RED);
  draw_time=HAL_GetTick();
  ST7789_DrawCircle(60, 60, 25, WHITE);
  printTime();
  HAL_Delay(1000);

  ST7789_Fill_Color(RED);
  ST7789_WriteString(10, 10, "Filled Circle.", Font_11x18, YELLOW, RED);
  draw_time=HAL_GetTick();
  ST7789_DrawFilledCircle(60, 60, 25, WHITE);
  printTime();
  printTime();
  HAL_Delay(1000);

  ST7789_Fill_Color(RED);
  ST7789_WriteString(10, 10, "Triangle.", Font_11x18, YELLOW, RED);
  draw_time=HAL_GetTick();
  ST7789_DrawTriangle(30, 30, 30, 70, 60, 40, WHITE);
  printTime();
  HAL_Delay(1000);

  ST7789_Fill_Color(RED);
  draw_time=HAL_GetTick();
  ST7789_WriteString(10, 10, "Filled Triangle.", Font_11x18, YELLOW, RED);
  ST7789_DrawFilledTriangle(30, 30, 30, 70, 60, 40, WHITE);
  printTime();
  HAL_Delay(1000);

  //  If FLASH cannot storage anymore datas, please delete codes below.
  ST7789_Fill_Color(WHITE);
  draw_time=HAL_GetTick();
  ST7789_DrawImage((ST7789_WIDTH-fry.width)/2, (ST7789_HEIGHT-fry.height)/2, fry.width, fry.height, fry.data);
  printTime();
  HAL_Delay(3000);
}
