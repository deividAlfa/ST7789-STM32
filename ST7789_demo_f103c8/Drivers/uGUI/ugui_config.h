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

#define USE_FONT_arial_6X6
#define USE_FONT_arial_9X9
#define USE_FONT_arial_9X12     // Includes cyrillic (UTF8 required)
#define USE_FONT_arial_15X15
#define USE_FONT_arial_15X17    // Includes cyrillic (UTF8 required)
#define USE_FONT_arial_19X20
#define USE_FONT_arial_19X23    // Includes cyrillic (UTF8 required)
#define USE_FONT_arial_25X24
#define USE_FONT_arial_25X28    // Includes cyrillic (UTF8 required)
#define USE_FONT_arial_29X29
#define USE_FONT_arial_29X34    // Includes cyrillic (UTF8 required)
#define USE_FONT_arial_35X34
#define USE_FONT_arial_35X39    // Includes cyrillic (UTF8 required)
#define USE_FONT_arial_39X40
#define USE_FONT_arial_39X45    // Includes cyrillic (UTF8 required)
#define USE_FONT_arial_45X45
#define USE_FONT_arial_45X53    // Includes cyrillic (UTF8 required)
#define USE_FONT_arial_49X49
#define USE_FONT_arial_49X57    // Includes cyrillic (UTF8 required)

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
