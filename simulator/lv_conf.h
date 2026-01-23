/**
 * @file lv_conf.h
 * Configuration file for LVGL v9
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

// Enable SVG and vector graphics support
#define LV_USE_FLOAT 1
#define LV_USE_MATRIX 1
#define LV_USE_VECTOR_GRAPHIC 1
#define LV_USE_SVG 1
#define LV_USE_THORVG_INTERNAL 1

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 8, 16, 24, or 32 */
#define LV_COLOR_DEPTH 32

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* Size of the memory available for `lv_malloc()` in bytes (>= 2kB) */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64U * 1024U)  /* 64 KB */

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period. */
#define LV_DEF_REFR_PERIOD 33  /* 33ms = ~30 FPS */

/* Input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30

/* Disable ARM-specific ASM optimizations for x86_64 */
#define LV_USE_DRAW_SW_ASM 0

/*====================
   FILESYSTEM
 *====================*/

/* Enable POSIX filesystem driver for simulator */
#define LV_USE_FS_POSIX 1
#if LV_USE_FS_POSIX
#define LV_FS_POSIX_LETTER 'P'
#define LV_FS_POSIX_PATH   ""
#endif

/*=================
   LOG SETTINGS
 *=================*/

/* Enable the log module */
#define LV_USE_LOG 1
#if LV_USE_LOG

/* Log level: TRACE, INFO, WARN, ERROR, USER, NONE */
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/* Use printf for logging */
#define LV_LOG_PRINTF 1

#endif  /* LV_USE_LOG */

/*================
   FONT USAGE
 *================*/

/* Montserrat fonts */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/* Use default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_16

/*===================
   WIDGET USAGE
 *===================*/

/* Enable widgets */
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      1

/*==================
   OTHERS
 *==================*/

/* Enable demo mode */
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0

#endif /* LV_CONF_H */
