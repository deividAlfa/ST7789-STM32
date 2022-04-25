#include "images.h"

// Sample image of 2x2px
// For the picture: https://sourceforge.net/projects/lcd-image-converter
// Image settings: Preset Color R5G6B5, Block size 16, Little-Endian, rest is left with default
const uint16_t sample_data [] = {
    0x4b10, 0x4b10, 0x4b10, 0x4b10
};

UG_BMP sample = {
  .p=sample_data,
  .width=2,
  .height=2,
  .bpp=BMP_BPP_16,
};

