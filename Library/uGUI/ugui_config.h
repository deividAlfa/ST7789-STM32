#ifndef __UGUI_CONFIG_H
#define __UGUI_CONFIG_H

#include <stdint.h>

/* -------------------------------------------------------------------------------- */
/* -- CONFIG SECTION                                                             -- */
/* -------------------------------------------------------------------------------- */

//#define USE_MULTITASKING    

/* Enable color mode */
#define USE_COLOR_RGB565

// If you only use standard ASCII, disabling this will save some resources
#define USE_UTF8

/* Enable needed fonts here */

//#define USE_FONT_4X16
//#define USE_FONT_5X8
//#define USE_FONT_5X12
//#define USE_FONT_6X8
//#define USE_FONT_6X10
//#define USE_FONT_7X12
//#define USE_FONT_8X8
//#define USE_FONT_8X12
//#define USE_FONT_8X12_CYRILLIC
//#define USE_FONT_10X16
//#define USE_FONT_12X16
//#define USE_FONT_12X20
//#define USE_FONT_16X26
//#define USE_FONT_22X36
//#define USE_FONT_24X40
//#define USE_FONT_32X53

//#define USE_FONT_arial_6X6
//#define USE_FONT_arial_7X8
//#define USE_FONT_arial_9X9
//#define USE_FONT_arial_11X12
//#define USE_FONT_arial_15X16
//#define USE_FONT_arial_19X20
//#define USE_FONT_arial_25X24
//#define USE_FONT_arial_31X31
//#define USE_FONT_arial_45X45
//#define USE_FONT_arial_49X49
//#define USE_FONT_arial_6X6_CYRILLIC
//#define USE_FONT_arial_8X9_CYRILLIC
//#define USE_FONT_arial_9X12_CYRILLIC
//#define USE_FONT_arial_11X15_CYRILLIC
//#define USE_FONT_arial_15X18_CYRILLIC
//#define USE_FONT_arial_19X23_CYRILLIC
//#define USE_FONT_arial_25X28_CYRILLIC
//#define USE_FONT_arial_31X36_CYRILLIC
//#define USE_FONT_arial_45X52_CYRILLIC
//#define USE_FONT_arial_49X58_CYRILLIC

/* Specify platform-dependent integer types here */

typedef uint8_t      UG_U8;
typedef int8_t       UG_S8;
typedef uint16_t     UG_U16;
typedef int16_t      UG_S16;
typedef uint32_t     UG_U32;
typedef int32_t      UG_S32;
typedef const uint8_t UG_FONT;

/* -------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------- */


/* Feature enablers */
#define USE_PRERENDER_EVENT
#define USE_POSTRENDER_EVENT


#endif
