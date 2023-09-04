#include "lcd.h"


/* Arg count, CMD, Args if any */
#if defined USE_ST7735
const uint8_t init_cmd[] = {
    0,  CMD_SLPOUT,
//  3,  CMD_FRMCTR1, 0x01, 0x2C, 0x2D,                     // Standard frame rate
//  3,  CMD_FRMCTR2, 0x01, 0x2C, 0x2D,                     // Standard frame rate
//  6,  CMD_FRMCTR3, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,   // Standard frame rate
    3,  CMD_FRMCTR1, 0x01, 0x01, 0x01,                     // Max
    3,  CMD_FRMCTR2, 0x01, 0x01, 0x01,                     // Max
    6,  CMD_FRMCTR3, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,   // Max frame rate
    1,  CMD_INVCTR,  0x07,
    3,  CMD_PWCTR1,  0xA2, 0x02, 0x84,
    1,  CMD_PWCTR2,  0xC5,
    2,  CMD_PWCTR3,  0x0A, 0x00,
    2,  CMD_PWCTR4,  0x8A, 0x2A,
    2,  CMD_PWCTR5,  0x8A, 0xEE,
    1,  CMD_VMCTR1,  0x0E,
    1,  CMD_INVOFF,  0x00,
    1,  CMD_COLMOD,  0x05,
    2,  CMD_CASET,   0x00, LCD_WIDTH-1,
    2,  CMD_RASET,   0x00, LCD_HEIGHT-1,
    1,  CMD_MADCTL,  LCD_ROTATION_CMD,
    16, CMD_GMCTRP1, 0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10,
    16, CMD_GMCTRN1, 0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10,
    0,  CMD_NORON,
};
#elif defined USE_ST7789
const uint8_t init_cmd[] = {
    0,  CMD_SLPOUT,
    1,  CMD_COLMOD,  CMD_COLOR_MODE_16bit,
    5,  CMD_PORCTRL, 0x0C, 0x0C, 0x00, 0x33, 0x33,   // Standard porch
  //5,  CMD_PORCTRL, 0x01, 0x01, 0x00, 0x11, 0x11,   // Minimum porch (7% faster screen refresh rate)
    1,  CMD_GCTRL,   0x35,                           // Gate Control, Default value
    1,  CMD_VCOMS,   0x19,                           // VCOM setting 0.725v (default 0.75v for 0x20)
    1,  CMD_LCMCTRL, 0X2C,                           // LCMCTRL, Default value
    1,  CMD_VDVVRHEN,0x01,                           // VDV and VRH command Enable, Default value
    1,  CMD_VRHS,    0x12,                           // VRH set, +-4.45v (default +-4.1v for 0x0B)
    1,  CMD_VDVS,    0x20,                           // VDV set, Default value
    1,  CMD_FRCTRL2, 0x0F,                           // Frame rate control in normal mode, Default refresh rate (60Hz)
  //1,  CMD_FRCTRL2, 0x01,                           // Frame rate control in normal mode, Max refresh rate (111Hz)
    2,  CMD_PWCTRL1, 0xA4, 0xA1,
    1,  CMD_MADCTL,  LCD_ROTATION_CMD,
    14, CMD_GMCTRP1, 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23,
    14, CMD_GMCTRN1, 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23,
    0,  CMD_INVON,
    0,  CMD_NORON
};
#endif


static void LCD_Update(void);
typedef struct{
  int8_t spi_sz;
  int8_t dma_sz;
  int8_t dma_mem_inc;
}config_t;

config_t config = {
    .spi_sz = -1,
    .dma_sz = -1,
    .dma_mem_inc = -1,
};

#ifdef LCD_LOCAL_FB
static uint16_t fb[LCD_WIDTH*LCD_HEIGHT];
#endif

static UG_GUI gui;
static UG_DEVICE device = {
    .x_dim = LCD_WIDTH,
    .y_dim = LCD_HEIGHT,
#ifdef LCD_LOCAL_FB
    .pset = LCD_DrawPixelFB,
#else
    .pset = LCD_DrawPixel,
#endif
    .flush = LCD_Update,
};

#define mode_16bit    1
#define mode_8bit     0
/*
 * @brief Sets SPI interface word size (0=8bit, 1=16 bit)
 * @param none
 * @return none
 */

static void setSPI_Size(int8_t size){
  if(config.spi_sz!=size){
    __HAL_SPI_DISABLE(&LCD_HANDLE);
    config.spi_sz=size;
    if(size==mode_16bit){
      LCD_HANDLE.Init.DataSize = SPI_DATASIZE_16BIT;
      LCD_HANDLE.Instance->CR1 |= SPI_CR1_DFF;
    }
    else{
      LCD_HANDLE.Init.DataSize = SPI_DATASIZE_8BIT;
      LCD_HANDLE.Instance->CR1 &= ~(SPI_CR1_DFF);
    }
  }
}


#ifdef USE_DMA
#define DMA_Min_Pixels    32             // Don't use DMA for small transfers? Setting this to 1 will always use DMA
#define mem_increase      1
#define mem_fixed         0

/**
 * @brief Configures DMA/ SPI interface
 * @param memInc Enable/disable memory address increase
 * @param mode16 Enable/disable 16 bit mode (disabled = 8 bit)
 * @return none
 */
static void setDMAMemMode(uint8_t memInc, uint8_t size)
{
  setSPI_Size(size);
  if(config.dma_sz!=size || config.dma_mem_inc!=memInc){
    config.dma_sz =size;
    config.dma_mem_inc = memInc;
    __HAL_DMA_DISABLE(LCD_HANDLE.hdmatx);;
#ifdef DMA_SxCR_EN
    while((LCD_HANDLE.hdmatx->Instance->CR & DMA_SxCR_EN) != RESET);
#elif defined DMA_CCR_EN
    while((LCD_HANDLE.hdmatx->Instance->CCR & DMA_CCR_EN) != RESET);
#endif
    if(memInc==mem_increase){
      LCD_HANDLE.hdmatx->Init.MemInc = DMA_MINC_ENABLE;
#ifdef DMA_SxCR_EN
      LCD_HANDLE.hdmatx->Instance->CR |= DMA_SxCR_MINC;
#elif defined DMA_CCR_EN
      LCD_HANDLE.hdmatx->Instance->CCR |= DMA_CCR_MINC;
#endif
    }
    else{
      LCD_HANDLE.hdmatx->Init.MemInc = DMA_MINC_DISABLE;
#ifdef DMA_SxCR_EN
      LCD_HANDLE.hdmatx->Instance->CR &= ~(DMA_SxCR_MINC);
#elif defined DMA_CCR_EN
      LCD_HANDLE.hdmatx->Instance->CCR &= ~(DMA_CCR_MINC);
#endif
    }

    if(size==mode_16bit){
      LCD_HANDLE.hdmatx->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
      LCD_HANDLE.hdmatx->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
#ifdef DMA_SxCR_EN
      LCD_HANDLE.hdmatx->Instance->CR = (LCD_HANDLE.hdmatx->Instance->CR & ~(DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk)) |
                                                   (1<<DMA_SxCR_PSIZE_Pos | 1<<DMA_SxCR_MSIZE_Pos);
#elif defined DMA_CCR_EN
      LCD_HANDLE.hdmatx->Instance->CCR = (LCD_HANDLE.hdmatx->Instance->CCR & ~(DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk)) |
                                                   (1<<DMA_CCR_PSIZE_Pos | 1<<DMA_CCR_MSIZE_Pos);
#endif

    }
    else{
      LCD_HANDLE.hdmatx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
      LCD_HANDLE.hdmatx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
#ifdef DMA_SxCR_EN
      LCD_HANDLE.hdmatx->Instance->CR = (LCD_HANDLE.hdmatx->Instance->CR & ~(DMA_SxCR_PSIZE_Msk | DMA_SxCR_MSIZE_Msk));
#elif defined DMA_CCR_EN
      LCD_HANDLE.hdmatx->Instance->CCR = (LCD_HANDLE.hdmatx->Instance->CCR & ~(DMA_CCR_PSIZE_Msk | DMA_CCR_MSIZE_Msk));
#endif
    }
  }
}
#endif

/**
 * @brief Write command to ST7735 controller
 * @param cmd -> command to write
 * @return none
 */
static void LCD_WriteCommand(uint8_t *cmd, uint8_t argc)
{
  setSPI_Size(mode_8bit);
  LCD_PIN(LCD_DC,RESET);
#ifdef LCD_CS
  LCD_PIN(LCD_CS,RESET);
#endif
  HAL_SPI_Transmit(&LCD_HANDLE, cmd, 1, HAL_MAX_DELAY);
  if(argc){
    LCD_PIN(LCD_DC,SET);
    HAL_SPI_Transmit(&LCD_HANDLE, (cmd+1), argc, HAL_MAX_DELAY);
  }
#ifdef LCD_CS
  LCD_PIN(LCD_CS,SET);
#endif
}

/**
 * @brief Write data to ST7735 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static void LCD_WriteData(uint8_t *buff, size_t buff_size)
{
  LCD_PIN(LCD_DC,SET);
#ifdef LCD_CS
  LCD_PIN(LCD_CS,RESET);
#endif

  // split data in small chunks because HAL can't send more than 64K at once

  while (buff_size > 0) {
    uint16_t chunk_size = buff_size > 65535 ? 65535 : buff_size;
#ifdef USE_DMA
    if(buff_size>DMA_Min_Pixels){
      HAL_SPI_Transmit_DMA(&LCD_HANDLE, buff, chunk_size);
      while(HAL_DMA_GetState(LCD_HANDLE.hdmatx)!=HAL_DMA_STATE_READY);
      if(config.dma_mem_inc==mem_increase){
        if(config.dma_sz==mode_16bit)
          buff += chunk_size;
        else
          buff += chunk_size*2;
      }
    }
    else{
      HAL_SPI_Transmit(&LCD_HANDLE, buff, chunk_size, HAL_MAX_DELAY);
      if(config.spi_sz==mode_16bit)
        buff += chunk_size;
      else
        buff += chunk_size*2;
    }
#else
    HAL_SPI_Transmit(&LCD_HANDLE, buff, chunk_size, HAL_MAX_DELAY);
#endif
    buff_size -= chunk_size;
  }
#ifdef LCD_CS
  LCD_PIN(LCD_CS,SET);
#endif
}

/**
 * @brief Write data to ST7735 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */

static void LCD_ReadCmd(uint8_t cmd, uint8_t *data, uint8_t count)
{
  setSPI_Size(mode_8bit);
  LCD_PIN(LCD_CS,RESET);
  LCD_PIN(LCD_DC,RESET);
  HAL_SPI_Transmit(&LCD_HANDLE, &cmd, sizeof(cmd), HAL_MAX_DELAY);
  LCD_PIN(LCD_DC,SET);
  HAL_SPI_Receive(&LCD_HANDLE, data, count, HAL_MAX_DELAY);
  LCD_PIN(LCD_CS,SET);
}

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in ST7735.h)
 * @return none
 */
void LCD_SetRotation(uint8_t m)
{
  uint8_t cmd[] = { CMD_MADCTL, 0};

  m = m % 4; // can't be higher than 3

  switch (m)
  {
  case 0:
#if LCD_IS_160X80
    cmd[1] = CMD_MADCTL_MX | CMD_MADCTL_MY | CMD_MADCTL_BGR;
#else
    cmd[1] = CMD_MADCTL_MX | CMD_MADCTL_MY | CMD_MADCTL_RGB;
#endif
    break;
  case 1:
#if CMD_IS_160X80
    cmd[1] = CMD_MADCTL_MY | CMD_MADCTL_MV | CMD_MADCTL_BGR;
#else
    cmd[1] = CMD_MADCTL_MY | CMD_MADCTL_MV | CMD_MADCTL_RGB;
#endif
    break;
  case 2:
#if CMD_IS_160X80
    cmd[1] = CMD_MADCTL_BGR;
#else
    cmd[1] = CMD_MADCTL_RGB;
#endif
    break;
  case 3:
#if CMD_IS_160X80
    cmd[1] = CMD_MADCTL_MX | CMD_MADCTL_MV | CMD_MADCTL_BGR;
#else
    cmd[1] = CMD_MADCTL_MX | CMD_MADCTL_MV | CMD_MADCTL_RGB;
#endif
    break;
  }
  LCD_WriteCommand(cmd, sizeof(cmd)-1);
}


/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void LCD_SetAddressWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  int16_t x_start = x0 + LCD_X_SHIFT, x_end = x1 + LCD_X_SHIFT;
  int16_t y_start = y0 + LCD_Y_SHIFT, y_end = y1 + LCD_Y_SHIFT;

  /* Column Address set */
  {
    uint8_t cmd[] = { CMD_CASET, x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF };
    LCD_WriteCommand(cmd, sizeof(cmd)-1);
  }
  /* Row Address set */
  {
    uint8_t cmd[] = { CMD_RASET, y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF };
    LCD_WriteCommand(cmd, sizeof(cmd)-1);
  }
  {
  /* Write to RAM */
    uint8_t cmd[] = { CMD_RAMWR };
    LCD_WriteCommand(cmd, sizeof(cmd)-1);
  }
}


/**
 * @brief Address and draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void LCD_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x > LCD_WIDTH-1) ||
     (y < 0) || (y > LCD_HEIGHT-1))
    return;

  uint8_t data[2] = {color >> 8, color & 0xFF};

  LCD_SetAddressWindow(x, y, x, y);

  LCD_PIN(LCD_DC,SET);
#ifdef LCD_CS
  LCD_PIN(LCD_CS,RESET);
#endif
  HAL_SPI_Transmit(&LCD_HANDLE, data, sizeof(data), HAL_MAX_DELAY);
#ifdef LCD_CS
  LCD_PIN(LCD_CS,SET);
#endif
}

#ifdef LCD_LOCAL_FB
void LCD_DrawPixelFB(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x >= LCD_WIDTH) ||
     (y < 0) || (y >= LCD_HEIGHT)) return;

  fb[x+(y*LCD_WIDTH)] = color;
}
#endif

void LCD_FillPixels(uint32_t pixels, uint16_t color){
#ifdef USE_DMA
  if(pixels>DMA_Min_Pixels)
    LCD_WriteData((uint8_t*)&color, pixels);
  else{
#endif
    uint16_t fill[DMA_Min_Pixels];                                                                // Use a pixel buffer for faster filling, removes overhead.
    for(uint32_t t=0;t<(pixels<DMA_Min_Pixels ? pixels : DMA_Min_Pixels);t++){                     // Fill the buffer with the color
      fill[t]=color;
    }
    while(pixels){                                                                                // Send 64 pixel blocks
      uint32_t sz = (pixels<DMA_Min_Pixels ? pixels : DMA_Min_Pixels);
      LCD_WriteData((uint8_t*)fill, sz);
      pixels-=sz;
    }
#ifdef USE_DMA
  }
#endif
}

/**
 * @brief Set address of DisplayWindow and returns raw pixel draw for uGUI driver acceleration
 * @param xi&yi -> coordinates of window
 * @return none
 */
void(*LCD_FillArea(int16_t x0, int16_t y0, int16_t x1, int16_t y1))(uint32_t, uint16_t){
  if(x0==-1){
#ifdef USE_DMA
    setDMAMemMode(mem_increase, mode_8bit);
#else
    setSPI_Size(mode_8bit);                                                          // Set SPI to 8 bit
#endif
    return NULL;
  }
  LCD_SetAddressWindow(x0,y0,x1,y1);
#ifdef USE_DMA
    setDMAMemMode(mem_fixed, mode_16bit);
#else
    setSPI_Size(mode_16bit);                                                          // Set SPI to 16 bit
#endif
  LCD_PIN(LCD_DC,SET);
  return LCD_FillPixels;
}


/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
int8_t LCD_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
  uint32_t pixels = (uint32_t)(xEnd-xSta+1)*(yEnd-ySta+1);
  LCD_SetAddressWindow(xSta, ySta, xEnd, yEnd);
#ifdef USE_DMA
    setDMAMemMode(mem_fixed, mode_16bit);
#else
    setSPI_Size(mode_16bit);
#endif
  LCD_FillPixels(pixels, color);
#ifdef USE_DMA
  setDMAMemMode(mem_increase, mode_8bit);
#else
  setSPI_Size(mode_8bit);
#endif
  return UG_RESULT_OK;
}


/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void LCD_DrawImage(uint16_t x, uint16_t y, UG_BMP* bmp)
{
  uint16_t w = bmp->width;
  uint16_t h = bmp->height;
  if ((x > LCD_WIDTH-1) || (y > LCD_HEIGHT-1))
    return;
  if ((x + w - 1) > LCD_WIDTH-1)
    return;
  if ((y + h - 1) > LCD_HEIGHT-1)
    return;
  if(bmp->bpp!=BMP_BPP_16)
    return;
  LCD_SetAddressWindow(x, y, x + w - 1, y + h - 1);

  #ifdef USE_DMA
  setDMAMemMode(mem_increase, mode_16bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
  #else
  setSPI_Size(mode_16bit);                                                                            // Set SPI to 16 bit
  #endif
  LCD_WriteData((uint8_t*)bmp->p, w*h);
#ifdef USE_DMA
setDMAMemMode(mem_increase, mode_8bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
#else
setSPI_Size(mode_8bit);                                                                            // Set SPI to 16 bit
#endif
  }

/**
 * @brief Accelerated line draw using filling (Only for vertical/horizontal lines)
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
int8_t LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {

  if(x0==x1){                                   // If horizontal
    if(y0>y1) swap(y0,y1);
  }
  else if(y0==y1){                              // If vertical
    if(x0>x1) swap(x0,x1);
  }
  else{                                         // Else, return fail, draw using software
    return UG_RESULT_FAIL;
  }

  LCD_Fill(x0,y0,x1,y1,color);               // Draw using acceleration
  return UG_RESULT_OK;
}
void LCD_PutChar(uint16_t x, uint16_t y, char ch, UG_FONT* font, uint16_t color, uint16_t bgcolor){
  UG_FontSelect(font);
  UG_PutChar(ch, x, y, color, bgcolor);
}

void LCD_PutStr(uint16_t x, uint16_t y,  char *str, UG_FONT* font, uint16_t color, uint16_t bgcolor){
  UG_FontSelect(font);
  UG_SetForecolor(color);
  UG_SetBackcolor(bgcolor);
  UG_PutString(x, y, str);
}
/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void LCD_InvertColors(uint8_t invert)
{
  uint8_t cmd[] = { (invert ? CMD_INVON /* INVON */ : CMD_INVOFF /* INVOFF */) };
  LCD_WriteCommand(cmd, sizeof(cmd)-1);
}

/*
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void LCD_TearEffect(uint8_t tear)
{
  uint8_t cmd[] = { (tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */) };
  LCD_WriteCommand(cmd, sizeof(cmd)-1);
}

void LCD_setPower(uint8_t power)
{
  uint8_t cmd[] = { (power ? CMD_DISPON /* TEON */ : CMD_DISPOFF /* TEOFF */) };
  LCD_WriteCommand(cmd, sizeof(cmd)-1);
}

static void LCD_Update(void)
{
#ifdef LCD_LOCAL_FB
  setSPI_Size(mode_8bit);
  LCD_SetAddressWindow(0,0,LCD_WIDTH-1,LCD_HEIGHT-1);
  #ifdef USE_DMA
  setDMAMemMode(mem_increase, mode_16bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
  #else
  setSPI_Size(mode_16bit);                                                                            // Set SPI to 16 bit
  #endif
  LCD_WriteData((uint8_t*)fb, LCD_WIDTH*LCD_HEIGHT);
#endif
  #ifdef USE_DMA
  setDMAMemMode(mem_increase, mode_8bit);                                                            // Set SPI and DMA to 16 bit, enable memory increase
  #else
  setSPI_Size(mode_8bit);                                                                            // Set SPI to 16 bit
  #endif
}
/**
 * @brief Initialize ST7735 controller
 * @param none
 * @return none
 */

void LCD_init(void)
{
#ifdef LCD_CS
  LCD_PIN(LCD_CS,SET);
#endif
#ifdef LCD_RST
  LCD_PIN(LCD_RST,RESET);
  HAL_Delay(1);
  LCD_PIN(LCD_RST,SET);
  HAL_Delay(200);
#endif
  UG_Init(&gui, &device);
#ifndef LCD_LOCAL_FB
  UG_DriverRegister(DRIVER_DRAW_LINE, LCD_DrawLine);
  UG_DriverRegister(DRIVER_FILL_FRAME, LCD_Fill);
  UG_DriverRegister(DRIVER_FILL_AREA, LCD_FillArea);
  UG_DriverRegister(DRIVER_DRAW_BMP, LCD_DrawImage);
#endif
  UG_FontSetHSpace(0);
  UG_FontSetVSpace(0);
  for(uint16_t i=0; i<sizeof(init_cmd); ){
    LCD_WriteCommand((uint8_t*)&init_cmd[i+1], init_cmd[i]);
    i += init_cmd[i]+2;
  }
  UG_FillScreen(C_BLACK);               //  Clear screen
  LCD_setPower(ENABLE);
  UG_Update();
}
