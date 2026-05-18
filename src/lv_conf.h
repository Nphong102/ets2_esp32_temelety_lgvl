#if 1

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

// ── Color ────────────────────────────────────────────────────
#define LV_COLOR_DEPTH     16
// ── Memory ──────────────────────────────────────────────────
#define LV_MEM_CUSTOM       0
#define LV_MEM_SIZE        (64U * 1024U)   // 64KB

// ── Tick: dùng millis() ─────────────────────────────────────
#define LV_TICK_CUSTOM      1
#define LV_TICK_CUSTOM_INCLUDE       "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

// ── Resolution ──────────────────────────────────────────────
#define LV_HOR_RES_MAX     240
#define LV_VER_RES_MAX     320

// ── Logging ─────────────────────────────────────────────────
#define LV_USE_LOG          0

// ── Fonts: chỉ bật size cần dùng ───────────────────────────
#define LV_FONT_MONTSERRAT_10    1
#define LV_FONT_MONTSERRAT_12    1
#define LV_FONT_MONTSERRAT_16    1
#define LV_FONT_MONTSERRAT_20    1
#define LV_FONT_MONTSERRAT_48    1  // speed digit
#define LV_FONT_DEFAULT        &lv_font_montserrat_12
#define LV_FONT_CUSTOM_DECLARE LV_FONT_DECLARE(obitron)
// ── Widgets: chỉ bật cần dùng ──────────────────────────────
#define LV_USE_ARC          0
#define LV_USE_BAR          1
#define LV_USE_BTN          0
#define LV_USE_BTNMATRIX    0
#define LV_USE_CANVAS       0
#define LV_USE_CHECKBOX     0
#define LV_USE_DROPDOWN     0
#define LV_USE_IMG          1
#define LV_USE_LABEL        1
#define LV_USE_LINE         1
#define LV_USE_ROLLER       0
#define LV_USE_SLIDER       0
#define LV_USE_SWITCH       0
#define LV_USE_TEXTAREA     0
#define LV_USE_TABLE        0
#define LV_USE_CHART        0
#define LV_USE_METER        0
#define LV_USE_MSGBOX       0
#define LV_USE_SPINBOX      0
#define LV_USE_SPINNER      0
#define LV_USE_TABVIEW      0
#define LV_USE_TILEVIEW     0
#define LV_USE_WIN          0
#define LV_USE_SPAN         0
#define LV_USE_LED          0
#define LV_USE_KEYBOARD     0
#define LV_USE_LIST         0
#define LV_USE_MENU         0
#define LV_USE_COLORWHEEL   0
#define LV_USE_CALENDAR     0

// ── Animation ───────────────────────────────────────────────
#define LV_USE_ANIMATION    1

// ── Draw ────────────────────────────────────────────────────
#define LV_DRAW_COMPLEX         1
#define LV_SHADOW_CACHE_SIZE    0
#define LV_IMG_CACHE_DEF_SIZE   0

// ── GPU (không có trên ESP32) ───────────────────────────────
#define LV_USE_GPU_STM32_DMA2D  0
#define LV_USE_GPU_NXP_PXP      0
#define LV_USE_GPU_NXP_VG_LITE  0
#define LV_USE_GPU_SDL          0

// ── Performance monitor (tắt production) ───────────────────
#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR      0

// ── Misc ────────────────────────────────────────────────────
#define LV_USE_USER_DATA        1
#define LV_SPRINTF_CUSTOM       0
#define LV_USE_ASSERT_NULL           0
#define LV_USE_ASSERT_MALLOC         0
#define LV_USE_ASSERT_STYLE          0
#define LV_USE_ASSERT_MEM_INTEGRITY  0
#define LV_USE_ASSERT_OBJ            0

#define LV_INDEV_DEF_SCROLL_LIMIT        10
#define LV_INDEV_DEF_SCROLL_THROW        10
#define LV_INDEV_DEF_LONG_PRESS_TIME    400
#define LV_INDEV_DEF_LONG_PRESS_REP_TIME 100
#define LV_INDEV_DEF_GESTURE_LIMIT       50
#define LV_INDEV_DEF_GESTURE_MIN_VELOCITY 3
#define LV_LABEL_DEF_SCROLL_SPEED        40

#endif // LV_CONF_H
#endif // enable guard
