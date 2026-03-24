/**
 * @file lv_conf.h
 * Configuration file for LVGL
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* Color depth: 1 (1 bit), 8 (8 bit), 16 (16 bit), 32 (32 bit) */
#define LV_COLOR_DEPTH 16

/* Horizontal and vertical resolution of the display. */
#define LV_HOR_RES_MAX 240
#define LV_VER_RES_MAX 320

/* Enable anti-aliasing */
#define LV_ANTIALIAS 1

/* 1: use a custom malloc/free, 0: use the default malloc/free */
#define LV_MEM_CUSTOM 0

/* Size of the memory used by `lv_mem_alloc` in bytes (>= 2kB)*/
#define LV_MEM_SIZE (128U * 1024U)

/* Maximal data size to allocate for `lv_mem_alloc` in one call (>= 1kB)*/
#define LV_MEM_MAX_SIZE (32U * 1024U)

/* Use dynamic memory with `malloc` and `free` */
#define LV_MEM_POOL_INCLUDE_STDLIB 1

/* Garbage collector for objects created from strings (E.g. labels from format strings)*/
#define LV_GC_INCLUDE_STDLIB 0

/* Set number of priorities */
#define LV_DISP_DEF_REFR_PERIOD 30

/* Enable the log module */
#define LV_USE_LOG 0

/* Enable outline drawing */
#define LV_USE_OUTLINE 1

/* Enable pattern drawing */
#define LV_USE_PATTERN 1

/* Enable image zooming */
#define LV_USE_IMG_ZOOM_INTERPOL 1

/* Enable shadow effect */
#define LV_USE_SHADOW 1

/* Enable blur effect */
#define LV_USE_BLUR 1

/* Enable ORCA S-curve interpolation */
#define LV_BEZIER_VALUE_MAX 256

/* Use Freetype to print texts with kerning */
#define LV_USE_FREETYPE 0

/*Printf-like logging*/
#define LV_LOG_PRINTF 0

/* Enable Memory logging */
#define LV_LOG_MEM 0

/* Enable timer logging */
#define LV_LOG_TIMER 0

/* Enable area logging */
#define LV_LOG_AREA 0

/* Enable the theme to be opaque */
#define LV_COLOR_SCREEN_TRANSP 0

/* Chroma keying for images */
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)

/* Enable the layouts */
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/* Enable the widgets */
#define LV_USE_BTN 1
#define LV_USE_LABEL 1
#define LV_USE_IMG 1
#define LV_USE_LINE 1
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_ROLLER 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1
#define LV_USE_CANVAS 1
#define LV_USE_SPINBOX 1

/* Set default value of some attributes */
#define LV_THEME_DEFAULT_LIGHT 1

#endif /* LV_CONF_H */
