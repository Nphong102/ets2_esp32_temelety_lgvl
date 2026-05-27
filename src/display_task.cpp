/**
 * display_task.cpp
 * - Init TFT_eSPI + LVGL
 * - Tạo toàn bộ widget UI (giống layout ảnh chụp)
 * - Cập nhật widget từ truck/game/job/trailer data
 * - Chạy trong DisplayTask trên core 1
 *
 * Buffer cấp phát từ PSRAM (ESP32-S3 có 8MB PSRAM)
 * → không tốn RAM nội nên không bao giờ stack overflow
 */
// ====================== CONFIG LVGL ======================
#define SCREEN_WIDTH    320     // ← Thay bằng độ phân giải thực của màn hình bạn
#define SCREEN_HEIGHT   240     // ← Thay bằng độ phân giải thực của màn hình bạn
#include "config.h"
#include "data_types.h"
#include "display_task.h"
#include "font/myfont.h"
#include "icon/icon.h"
 /* forward-declare the symbol */
#if LV_FONT_MONTSERRAT_16
#pragma message "LV_FONT_MONTSERRAT_48 is enabled"
#else
#error "LV_FONT_MONTSERRAT_16 is NOT enabled!"
#endif
// ════════════════════════════════════════════════════════
// 1. LVGL DRIVER — buffer từ PSRAM
// ════════════════════════════════════════════════════════
#define BUF_LINES  20                        // 20 dòng × 240px × 2byte = 9,600B
#define BUF_SIZE   (240 * BUF_LINES)

static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t      disp_drv;

static lv_color_t *lvgl_buf1 = nullptr;     // cấp phát từ PSRAM trong setupDisplay()
static lv_color_t *lvgl_buf2 = nullptr;
// ====================== BOOT ANIMATION ======================
static lv_obj_t *boot_scr = nullptr;
static lv_obj_t *img_scr = nullptr;
static lv_obj_t *boot_label = nullptr;
static lv_obj_t *boot_progress = nullptr;
static lv_obj_t *boot_status = nullptr;
static lv_obj_t *test ;
void showBootSequence() {
    lv_obj_t *main_scr = lv_scr_act();

    // Tạo màn hình boot riêng
    boot_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(boot_scr, lv_color_hex(0x000010), 0);
    lv_obj_set_style_bg_opa(boot_scr, LV_OPA_COVER, 0);

    // Logo / Title
    lv_obj_t *title = lv_label_create(boot_scr);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x00DDFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    lv_label_set_text(title, "TRUCK DASHBOARD");

    lv_obj_t *subtitle = lv_label_create(boot_scr);
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x888888), 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 60);
    lv_label_set_text(subtitle, "v1.0 - ESP32-S3");

    // Progress bar
    boot_progress = lv_bar_create(boot_scr);
    lv_obj_set_size(boot_progress, 200, 12);
    lv_obj_align(boot_progress, LV_ALIGN_CENTER, 0, 20);
    lv_bar_set_range(boot_progress, 0, 100);
    lv_bar_set_value(boot_progress, 0, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(boot_progress, lv_color_hex(0x003366), LV_PART_MAIN);
    lv_obj_set_style_bg_color(boot_progress, lv_color_hex(0x00AAFF), LV_PART_INDICATOR);

    // Status text
    boot_status = lv_label_create(boot_scr);
    lv_obj_set_style_text_font(boot_status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(boot_status, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(boot_status, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_label_set_text(boot_status, "Initializing...");

    
    // Hiển thị logo 1
    lv_obj_t *logo_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(logo_scr, lv_color_black(), 0);
    lv_obj_t *logo_img = lv_img_create(logo_scr);
    lv_img_set_src(logo_img, &ribi1);
    lv_obj_center(logo_img);
    lv_scr_load_anim(logo_scr, LV_SCR_LOAD_ANIM_FADE_IN, 500, 0, false);
        Serial.printf("anh 1 load  psram=%d\n",ESP.getFreePsram());
    unsigned long t = millis();
    while (millis() - t < 2000) { lv_task_handler(); delay(5); }

 // Hiển thị logo 2
    lv_obj_t *scr_x = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr_x, lv_color_black(), 0);
    lv_obj_t *img_x = lv_img_create(scr_x);
    lv_img_set_src(img_x, &ribi2);
    lv_obj_center(img_x);
    lv_scr_load_anim(scr_x, LV_SCR_LOAD_ANIM_OVER_RIGHT, 500, 0, false);
     Serial.printf("anh 2  load  psram=%d\n",ESP.getFreePsram());
    t = millis();
    while (millis() - t < 2000) { lv_task_handler(); delay(5); }

    // Chuyển sang boot screen
    lv_scr_load_anim(boot_scr, LV_SCR_LOAD_ANIM_FADE_IN, 500, 0, false);
    t = millis();
    while (millis() - t < 600) { lv_timer_handler(); delay(5); }
    // Xóa logo screens ngay sau khi không dùng nữa
    lv_obj_del(logo_scr);
    lv_obj_del(scr_x);

    // ====================== BOOT SEQUENCE ======================
    const char* steps[] = {
        "Loading LVGL...",
        "Initializing TFT...",
        "Connecting to Truck ECU...",
        "Loading truck data...",
        "Loading trailer data...",
        "Checking systems...",
        "Ready"
    };

    for (int i = 0; i <= 100; i += 10) {
        lv_bar_set_value(boot_progress, i, LV_ANIM_ON);
        
        int step = i / 15;
        if (step < 7) {
            lv_label_set_text(boot_status, steps[step]);
        }

        // Hiển thị một số thông số ngẫu nhiên (có thể thay bằng data thật)
        if (i == 40) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Truck: %s %s", truck.make.c_str(), truck.model.c_str());
            lv_label_set_text(boot_status, buf);
        }
        if (i == 70) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Plate: %s | ODO: %dkm", 
                     truck.licensePlate.c_str(), truck.odo);
            lv_label_set_text(boot_status, buf);
        }

        lv_task_handler();
        delay(180); // Tăng/giảm để thay đổi tốc độ boot
    }

    // Boot hoàn tất → chuyển sang màn hình chính
    lv_label_set_text(boot_status, "System Ready ✓");
    lv_task_handler();
    delay(800);

     lv_scr_load_anim(main_scr, LV_SCR_LOAD_ANIM_FADE_IN, 500, 0, false);
    t = millis();
    while (millis() - t < 600) { lv_timer_handler(); delay(5); }
    
    lv_obj_del(boot_scr);
    boot_scr = nullptr;
    Serial.println("[BOOT] Boot sequence completed");
}
void lvgl_create_buffers() 
{
    // Chọn kích thước buffer (càng lớn càng mượt, nhưng tốn PSRAM)
    // BUF_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT / N
    const uint32_t buf_size = (SCREEN_WIDTH * SCREEN_HEIGHT) ;   // ← Dùng 1/4 màn hình (khuyến nghị)

    Serial.printf("[LVGL] Allocating buffers: %d pixels each (~%d KB)\n", 
                  buf_size, (buf_size * sizeof(lv_color_t)) / 1024);

    // Cấp phát từ PSRAM
    lvgl_buf1 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), 
                                              MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    
    lvgl_buf2 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), 
                                              MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (lvgl_buf1 == nullptr || lvgl_buf2 == nullptr) 
    {
        Serial.println("[LVGL] PSRAM alloc failed! Using smaller buffer.");

        // Fallback: dùng buffer nhỏ hơn trong Internal RAM
        if (lvgl_buf1) heap_caps_free(lvgl_buf1);
        if (lvgl_buf2) heap_caps_free(lvgl_buf2);

        lvgl_buf1 = (lv_color_t*)heap_caps_malloc(SCREEN_WIDTH * 40 * sizeof(lv_color_t), MALLOC_CAP_INTERNAL);
        lvgl_buf2 = nullptr;

        lv_disp_draw_buf_init(&draw_buf, lvgl_buf1, lvgl_buf2, SCREEN_WIDTH * 40);
        Serial.println("[LVGL] Fallback to small internal buffer");
    } 
    else 
    {
        lv_disp_draw_buf_init(&draw_buf, lvgl_buf1, lvgl_buf2, buf_size);
        Serial.println("[LVGL] Double buffer in PSRAM → OK (Good performance)");
    }
    }

// flush callback — LVGL → TFT_eSPI
static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)color_p, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(drv);
}

// ════════════════════════════════════════════════════════
// 2. WIDGET HANDLES
// ════════════════════════════════════════════════════════

// Header
static lv_obj_t *lbl_truck_name;   // "MAN TGX"
static lv_obj_t *lbl_plate;        // "77C-05669"
static lv_obj_t *lbl_gear;         // "4"
static lv_obj_t *lbl_clock;        // "10:13"

// Row 2: nhiệt độ + speed
static lv_obj_t *lbl_oil;          // "Oil:75C"
static lv_obj_t *lbl_h2o;          // "H2O:67C"
static lv_obj_t *lbl_speed;        // "0"  (font 48)

// Row 3: ODO + distance
static lv_obj_t *lbl_odo;
static lv_obj_t *lbl_dist;

// Row 4: bars
static lv_obj_t *bar_fuel;
static lv_obj_t *lbl_fuel_val;
static lv_obj_t *bar_air;
static lv_obj_t *lbl_air_val;

// Row 5: wear
static lv_obj_t *lbl_dam;          // "Dam Info" label
static lv_obj_t *lbl_wear_t;
static lv_obj_t *lbl_wear_tr;
static lv_obj_t *lbl_wear_c;

// Footer
static lv_obj_t *lbl_trailer;
static lv_obj_t *lbl_time;
static lv_obj_t *lbl_income;
static lv_obj_t *icon_park;
static lv_obj_t *lbl_trl_prefix;
static lv_obj_t *lbl_vol;
static lv_obj_t  *spd_limit ;
static const lv_img_dsc_t* speed_imgs[] = {&sp30, &sp40, &sp50, &sp60, &sp70, &sp80, &sp90};

// ════════════════════════════════════════════════════════
// 3. HELPER: tạo horizontal divider
// ════════════════════════════════════════════════════════
static void make_divider(lv_obj_t *parent, lv_coord_t y, lv_color_t color) {
    lv_obj_t *d = lv_obj_create(parent);
    lv_obj_set_size(d, 240, 2);
    lv_obj_set_pos(d, 0, y);
    lv_obj_set_style_bg_color(d, color, 0);
    lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(d, 0, 0);
    lv_obj_set_style_radius(d, 0, 0);
    lv_obj_set_style_pad_all(d, 0, 0);
    lv_obj_clear_flag(d, LV_OBJ_FLAG_SCROLLABLE);
}

// ════════════════════════════════════════════════════════
// 4. createUI — xây widget tree một lần duy nhất
// ════════════════════════════════════════════════════════
static void createUI() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000010), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // ── Header y=0..38 ────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, 240, 38);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x000010), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_all(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lbl_truck_name = lv_label_create(hdr);
    lv_obj_set_style_text_font(lbl_truck_name, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_truck_name, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(lbl_truck_name, 4, 2);
    lv_label_set_text(lbl_truck_name, "MAN TGX");

    lbl_plate = lv_label_create(hdr);
    lv_obj_set_style_text_font(lbl_plate, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_plate, lv_color_hex(0xFFD700), 0);
    lv_obj_set_pos(lbl_plate, 4, 20);
    lv_label_set_text(lbl_plate, "---");

    lbl_gear = lv_label_create(hdr);
    lv_obj_set_style_text_font(lbl_gear, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_gear, lv_color_hex(0xFFD700), 0);
    lv_obj_align(lbl_gear, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(lbl_gear, "N");

    lbl_clock = lv_label_create(hdr);
    lv_obj_set_style_text_font(lbl_clock, &roboto, 0);
    lv_obj_set_style_text_color(lbl_clock, lv_color_hex(0xFFD700), 0);
    lv_obj_align(lbl_clock, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_label_set_text(lbl_clock, "--:--");

    lbl_vol = lv_label_create(hdr);
    lv_obj_set_style_text_font(lbl_vol, &roboto, 0);
    lv_obj_set_style_text_color(lbl_vol, lv_color_hex(0xFFD700), 0);
    lv_obj_align(lbl_vol, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_label_set_text(lbl_vol, "V");
    // ── Divider xanh y=38 ─────────────────────────────────────
    make_divider(scr, 38, lv_color_hex(0x3A6EA5));

    // ── Row2: Temp + Speed  y=40..99 ──────────────────────────
    lbl_oil = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_oil,  &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_oil, lv_color_hex(0xFF9900), 0);
    lv_obj_set_pos(lbl_oil, 4, 44);
    lv_label_set_text(lbl_oil, "Oil:--C");

    lbl_h2o = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_h2o,  &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_h2o, lv_color_hex(0x00AAFF), 0);
    lv_obj_set_pos(lbl_h2o, 4, 62);
    lv_label_set_text(lbl_h2o, "H2O:--C");

    lbl_speed = lv_label_create(scr);
   // lv_obj_set_style_text_font(lbl_speed, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lbl_speed, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl_speed, &obitron, 0);
    lv_obj_align(lbl_speed, LV_ALIGN_TOP_MID, 0, 45);
    lv_label_set_text(lbl_speed, "0");

    // ── Row3: ODO + Dist  y=100..112 ──────────────────────────
    lbl_odo = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_odo, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_odo, lv_color_hex(0xFFD700), 0);
    lv_obj_set_pos(lbl_odo, 4, 100);
    lv_label_set_text(lbl_odo, "ODO:0km");

    lbl_dist = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_dist, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_dist, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_dist, LV_ALIGN_TOP_RIGHT, -4, 100);
    lv_label_set_text(lbl_dist, "0km");

    // ── Row4: Fuel bar + Air bar  y=114..132 ──────────────────
    // Fuel label icon
    lv_obj_t *ic_fuel = lv_label_create(scr);
    lv_obj_set_style_text_font(ic_fuel, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ic_fuel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(ic_fuel, 2, 115);
    lv_label_set_text(ic_fuel, LV_SYMBOL_BATTERY_FULL);

    bar_fuel = lv_bar_create(scr);
    lv_obj_set_size(bar_fuel, 94, 14);
    lv_obj_set_pos(bar_fuel, 20, 116);
    lv_bar_set_range(bar_fuel, 0, 1000);
    lv_bar_set_value(bar_fuel, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_fuel, lv_color_hex(0x0A0A2A), LV_PART_MAIN);
    lv_obj_set_style_border_color(bar_fuel, lv_color_hex(0x2244AA), LV_PART_MAIN);
    lv_obj_set_style_border_width(bar_fuel, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_fuel, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_fuel, lv_color_hex(0x2266FF), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_fuel, 1, LV_PART_INDICATOR);

    lbl_fuel_val = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_fuel_val,  &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_fuel_val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align_to(lbl_fuel_val, bar_fuel, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(lbl_fuel_val, "0");

    // Air label icon
    lv_obj_t *ic_air = lv_label_create(scr);
    lv_obj_set_style_text_font(ic_air, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ic_air, lv_color_hex(0xFFD700), 0);
    lv_obj_set_pos(ic_air, 122, 115);
    lv_label_set_text(ic_air, LV_SYMBOL_REFRESH);

    bar_air = lv_bar_create(scr);
    lv_obj_set_size(bar_air, 88, 14);
    lv_obj_set_pos(bar_air, 142, 116);
    lv_bar_set_range(bar_air, 0, 150);
    lv_bar_set_value(bar_air, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_air, lv_color_hex(0x0A0A2A), LV_PART_MAIN);
    lv_obj_set_style_border_color(bar_air, lv_color_hex(0x336600), LV_PART_MAIN);
    lv_obj_set_style_border_width(bar_air, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(bar_air, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_air, lv_color_hex(0x88CC00), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_air, 1, LV_PART_INDICATOR);

    lbl_air_val = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_air_val,  &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_air_val, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align_to(lbl_air_val, bar_air, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(lbl_air_val, "0");

    // ── Row5: Dam Info + Wear  y=134..210 ─────────────────────
    lbl_dam = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_dam, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_dam, lv_color_hex(0x4499FF), 0);
    lv_obj_set_pos(lbl_dam, 2, 138);
    lv_label_set_text(lbl_dam, "Dam\nInfo");

    lbl_wear_t = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_wear_t,  &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_wear_t, lv_color_hex(0x88CC00), 0);
    lv_obj_set_pos(lbl_wear_t, 42, 150);
    lv_label_set_text(lbl_wear_t, "T:0%");

    lbl_wear_tr = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_wear_tr,  &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_wear_tr, lv_color_hex(0x88CC00), 0);
    lv_obj_align(lbl_wear_tr, LV_ALIGN_TOP_MID, 10, 150);
    lv_label_set_text(lbl_wear_tr, "Tr:0%");

    lbl_wear_c = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_wear_c,  &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_wear_c, lv_color_hex(0x88CC00), 0);
    lv_obj_align(lbl_wear_c, LV_ALIGN_TOP_RIGHT, -30, 150);
    lv_label_set_text(lbl_wear_c, "C:0%");

    icon_park = lv_img_create(scr);
    lv_obj_set_pos(icon_park, 200, 200); 
    lv_img_set_src(icon_park, &park);
    
 

    // ── Divider đỏ y=212 ──────────────────────────────────────
    make_divider(scr, 280, lv_color_hex(0xCC2200));

    // ── Footer  y=213..319 ────────────────────────────────────
    lbl_trl_prefix = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_trl_prefix, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_trl_prefix, lv_color_hex(0x88CC00), 0);
    lv_obj_set_pos(lbl_trl_prefix, 4, 282);
    lv_label_set_text(lbl_trl_prefix, "TRL:");

    lbl_trailer = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_trailer, &roboto, 0);
    lv_obj_set_style_text_color(lbl_trailer, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_pos(lbl_trailer, 36, 282);
    lv_obj_set_width(lbl_trailer, 200);
    lv_label_set_long_mode(lbl_trailer, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(lbl_trailer, "Không có hàng");

    lbl_time = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(lbl_time, "Time: --");
    lv_obj_align(lbl_time, LV_ALIGN_BOTTOM_LEFT, -4, 0);

    lbl_income = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_income, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_income, lv_color_hex(0xFFD700), 0);
    lv_label_set_text(lbl_income, "$:0");
    lv_obj_align(lbl_income, LV_ALIGN_BOTTOM_RIGHT, -4, 0);

    spd_limit = lv_img_create(scr);
    
    lv_img_set_src(spd_limit, &sp30);
    lv_obj_align_to(spd_limit,lbl_trl_prefix,LV_ALIGN_OUT_TOP_LEFT, -5, -10); 
    test = lv_label_create(scr);
    lv_obj_align_to(test,spd_limit,LV_ALIGN_OUT_RIGHT_TOP,5,5);
    lv_label_set_text(test, "0");


    Serial.println("[UI] Widgets created OK");
}

// ════════════════════════════════════════════════════════
// 5. wearColor helper
// ════════════════════════════════════════════════════════
static lv_color_t wearColor(int pct) {
    if (pct < 20) return lv_color_hex(0x88CC00);  // xanh
    if (pct < 60) return lv_color_hex(0xFFAA00);  // vàng
    return           lv_color_hex(0xFF4400);       // đỏ
}

// ════════════════════════════════════════════════════════
// 6. refreshUI — cập nhật widget từ data (gọi trong DisplayTask)
// ════════════════════════════════════════════════════════
static void refreshUI() {
    char buf[48];

    // ── Clock ─────────────────────────────────────────────────
    static String prev_clock;
    String cur_clock = game.realTime.length() > 0 ? game.realTime : String(timeStr);
    if (cur_clock != prev_clock) {
        prev_clock = cur_clock;
        lv_label_set_text(lbl_clock, cur_clock.c_str());
       
    }
   
    // ── Truck name + plate ────────────────────────────────────
    static String prev_name, prev_plate;
    String name = truck.make + " " + truck.model;
    if (name.length() > 14) name = name.substring(0, 13) + ".";
    if (name != prev_name) {
        prev_name = name;
        lv_label_set_text(lbl_truck_name, name.c_str());
    }
    String cur_plate = truck.licensePlate.length() > 0 ? truck.licensePlate : "---";
    if (cur_plate != prev_plate) {
        prev_plate = cur_plate;
        lv_label_set_text(lbl_plate, cur_plate.c_str());
    }

    // ── Gear ──────────────────────────────────────────────────
    static int prev_gear = -999;
    if (truck.displayedGear != prev_gear) {
        prev_gear = truck.displayedGear;
        if      (truck.displayedGear < 0)  snprintf(buf, sizeof(buf), "R%d", abs(truck.displayedGear));
        else if (truck.displayedGear == 0) snprintf(buf, sizeof(buf), "N");
        else                               snprintf(buf, sizeof(buf), "%d", truck.displayedGear);
        lv_label_set_text(lbl_gear, buf);
    }

    // ── Speed ─────────────────────────────────────────────────
    static int prev_speed = -1;
    static bool prev_speed_warn = false;
    bool speed_warn = truck.speedLimit > 0 && truck.speed > truck.speedLimit + 5;
    if (truck.speed != prev_speed) {
        prev_speed = truck.speed;
        snprintf(buf, sizeof(buf), "%d", truck.speed);
        lv_label_set_text(lbl_speed, buf);
    }
    if (speed_warn != prev_speed_warn) {
        prev_speed_warn = speed_warn;
        lv_obj_set_style_text_color(lbl_speed,
            speed_warn ? lv_color_hex(0xFF4444) : lv_color_hex(0xFFFFFF), 0);
    }

    // ── Oil ───────────────────────────────────────────────────
    static int prev_oil = -1;
    if (truck.oilTemperature != prev_oil) {
        prev_oil = truck.oilTemperature;
        snprintf(buf, sizeof(buf), "Oil:%dC", truck.oilTemperature);
        lv_label_set_text(lbl_oil, buf);
        lv_obj_set_style_text_color(lbl_oil,
            truck.oilTemperature > 105 ? lv_color_hex(0xFF4400) : lv_color_hex(0xFF9900), 0);
    }

    // ── H2O ───────────────────────────────────────────────────
    static int prev_h2o = -1;
    if (truck.waterTemperature != prev_h2o) {
        prev_h2o = truck.waterTemperature;
        snprintf(buf, sizeof(buf), "H2O:%dC", truck.waterTemperature);
        lv_label_set_text(lbl_h2o, buf);
        lv_obj_set_style_text_color(lbl_h2o,
            truck.waterTemperature > 95 ? lv_color_hex(0xFF4400) : lv_color_hex(0x00AAFF), 0);
    }

    // ── ODO ───────────────────────────────────────────────────
    static int prev_odo = -1;
    if (truck.odo != prev_odo) {
        prev_odo = truck.odo;
        snprintf(buf, sizeof(buf), "ODO:%dkm", truck.odo);
        lv_label_set_text(lbl_odo, buf);
    }

    // ── Distance ──────────────────────────────────────────────
    static int prev_dist = -1;
    if (navigation.estimatedDistance != prev_dist) {
        prev_dist = navigation.estimatedDistance;
        snprintf(buf, sizeof(buf), "%dkm", navigation.estimatedDistance);
        lv_label_set_text(lbl_dist, buf);
    }

    // ── Fuel bar ──────────────────────────────────────────────
    static int prev_fuel = -1;
    static bool prev_fuel_warn = false;
    if (truck.fuel != prev_fuel || truck.fuelWarningOn != prev_fuel_warn) {
        prev_fuel = truck.fuel;
        prev_fuel_warn = truck.fuelWarningOn;
        int fuelCap = max(1, truck.fuelCapacity);
        lv_bar_set_range(bar_fuel, 0, fuelCap);
        lv_bar_set_value(bar_fuel, truck.fuel, LV_ANIM_ON);
        snprintf(buf, sizeof(buf), "%d", truck.fuel);
        lv_label_set_text(lbl_fuel_val, buf);
        lv_obj_set_style_bg_color(bar_fuel,
            truck.fuelWarningOn ? lv_color_hex(0xFF4400) : lv_color_hex(0x2266FF),
            LV_PART_INDICATOR);
    }

    // ── Air bar ───────────────────────────────────────────────
    static int prev_air = -1;
    static bool prev_air_warn = false;
    if (truck.airPressure != prev_air || truck.airWarningOn != prev_air_warn) {
        prev_air = truck.airPressure;
        prev_air_warn = truck.airWarningOn;
        lv_bar_set_value(bar_air, truck.airPressure, LV_ANIM_ON);
        snprintf(buf, sizeof(buf), "%d", truck.airPressure);
        lv_label_set_text(lbl_air_val, buf);
        lv_obj_set_style_bg_color(bar_air,
            truck.airWarningOn ? lv_color_hex(0xFF4400) : lv_color_hex(0x88CC00),
            LV_PART_INDICATOR);
    }

    // ── Wear ──────────────────────────────────────────────────
    static int prev_wear_t = -1, prev_wear_tr = -1, prev_wear_c = -1;
    if (truck.wear != prev_wear_t) {
        prev_wear_t = truck.wear;
        snprintf(buf, sizeof(buf), "T:%d%%", truck.wear);
        lv_label_set_text(lbl_wear_t, buf);
        lv_obj_set_style_text_color(lbl_wear_t, wearColor(truck.wear), 0);
    }
    if (trailer.wear != prev_wear_tr) {
        prev_wear_tr = trailer.wear;
        snprintf(buf, sizeof(buf), "Tr:%d%%", trailer.wear);
        lv_label_set_text(lbl_wear_tr, buf);
        lv_obj_set_style_text_color(lbl_wear_tr, wearColor(trailer.wear), 0);
    }
    if (trailer.cargoWear != prev_wear_c) {
        prev_wear_c = trailer.cargoWear;
        snprintf(buf, sizeof(buf), "C:%d%%", trailer.cargoWear);
        lv_label_set_text(lbl_wear_c, buf);
        lv_obj_set_style_text_color(lbl_wear_c, wearColor(trailer.cargoWear), 0);
    }

    // ── Trailer ───────────────────────────────────────────────
    static String prev_trailer;
    String cur_trailer;
    if (trailer.attached && trailer.name.length() > 0) {
        int massT = trailer.mass / 1000;
        snprintf(buf, sizeof(buf), "%s (%dT)",
                 trailer.name.c_str(), massT);
        cur_trailer = buf;
    } else {
        cur_trailer = "--------Không có hàng hoá ------";
    }
    if (cur_trailer != prev_trailer) {
        prev_trailer = cur_trailer;
        lv_label_set_text(lbl_trailer, cur_trailer.c_str());
    }

    // ── Time remaining ────────────────────────────────────────
    static String prev_time;
    if (job.remainingTime.length() > 0 && job.remainingTime != prev_time) {
        prev_time = job.remainingTime;
        snprintf(buf, sizeof(buf), "Time: %s", job.remainingTime.c_str());
        lv_label_set_text(lbl_time, buf);
        lv_obj_set_style_text_color(lbl_time,
            job.remainingTime[0] == '-'
            ? lv_color_hex(0xFF4400) : lv_color_hex(0xFFFFFF), 0);
    }

    // ── Income ────────────────────────────────────────────────
    static int prev_income = -1;
    if (job.income != prev_income) {
        prev_income = job.income;
        snprintf(buf, sizeof(buf), "$:%d", job.income);
        lv_label_set_text(lbl_income, buf);
    }

    if (truck.parkingBreak == false){
        lv_obj_add_flag(icon_park, LV_OBJ_FLAG_HIDDEN);

    } else 
    {
        lv_obj_clear_flag(icon_park, LV_OBJ_FLAG_HIDDEN);
    }

     static int lass_spd_limit = 0;
     if (navigation.speed_limit != lass_spd_limit )
     {
        lass_spd_limit = navigation.speed_limit;
        // Gọi
            int idx = (navigation.speed_limit - 30) / 10;  // 30→0, 40→1, ...
            if (idx >= 0 && idx <= 6) {
                lv_img_set_src(spd_limit, speed_imgs[idx]);
            }
  
     }

    static unsigned long last_read_vol = 0;
    if (millis() - last_read_vol > 1000) {
        int adcValue = analogRead(BAT_ADC_PIN);
        float batteryVoltage = analogReadMilliVolts(BAT_ADC_PIN) * 2.0 / 1000.0;
        
         snprintf(buf, sizeof(buf), "%.2f V",batteryVoltage);
         lv_label_set_text(lbl_vol,buf);
        Serial.printf("Battery: %.2fV\n", batteryVoltage);
        last_read_vol = millis();
    }

}

static void refreshUIOLD() {
    char buf[48];

    // ── Clock ─────────────────────────────────────────────────
    lv_label_set_text(lbl_clock, game.realTime.length() > 0
                                 ? game.realTime.c_str() : timeStr);

    // ── Truck name + plate ────────────────────────────────────
    String name = truck.make + " " + truck.model;
    if (name.length() > 14) name = name.substring(0, 13) + ".";
    lv_label_set_text(lbl_truck_name, name.c_str());
    lv_label_set_text(lbl_plate,
        truck.licensePlate.length() > 0 ? truck.licensePlate.c_str() : "---");

    // ── Gear ──────────────────────────────────────────────────
    if      (truck.displayedGear < 0)  snprintf(buf, sizeof(buf), "R%d", abs(truck.displayedGear));
    else if (truck.displayedGear == 0) snprintf(buf, sizeof(buf), "N");
    else                               snprintf(buf, sizeof(buf), "%d", truck.displayedGear);
    lv_label_set_text(lbl_gear, buf);

    // ── Speed ─────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "%d", truck.speed);
    lv_label_set_text(lbl_speed, buf);
    lv_color_t sc = (truck.speedLimit > 0 && truck.speed > truck.speedLimit + 5)
                    ? lv_color_hex(0xFF4444) : lv_color_hex(0xFFFFFF);
    lv_obj_set_style_text_color(lbl_speed, sc, 0);

    // ── Oil ───────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "Oil:%dC", truck.oilTemperature);
    lv_label_set_text(lbl_oil, buf);
    lv_obj_set_style_text_color(lbl_oil,
        truck.oilTemperature > 105 ? lv_color_hex(0xFF4400) : lv_color_hex(0xFF9900), 0);

    // ── H2O ───────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "H2O:%dC", truck.waterTemperature);
    lv_label_set_text(lbl_h2o, buf);
    lv_obj_set_style_text_color(lbl_h2o,
        truck.waterTemperature > 95 ? lv_color_hex(0xFF4400) : lv_color_hex(0x00AAFF), 0);

    // ── ODO ───────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "ODO:%dkm", truck.odo);
    lv_label_set_text(lbl_odo, buf);

    // ── Distance ──────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "%dkm", navigation.estimatedDistance);
    lv_label_set_text(lbl_dist, buf);

    // ── Fuel bar ──────────────────────────────────────────────
    int fuelCap = max(1, truck.fuelCapacity);
    lv_bar_set_range(bar_fuel, 0, fuelCap);
    lv_bar_set_value(bar_fuel, truck.fuel, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%d", truck.fuel);
    lv_label_set_text(lbl_fuel_val, buf);
    lv_obj_set_style_bg_color(bar_fuel,
        truck.fuelWarningOn ? lv_color_hex(0xFF4400) : lv_color_hex(0x2266FF),
        LV_PART_INDICATOR);

    // ── Air bar ───────────────────────────────────────────────
    lv_bar_set_value(bar_air, truck.airPressure, LV_ANIM_ON);
    snprintf(buf, sizeof(buf), "%d", truck.airPressure);
    lv_label_set_text(lbl_air_val, buf);
    lv_obj_set_style_bg_color(bar_air,
        truck.airWarningOn ? lv_color_hex(0xFF4400) : lv_color_hex(0x88CC00),
        LV_PART_INDICATOR);

    // ── Wear ──────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "T:%d%%",  truck.wear);
    lv_label_set_text(lbl_wear_t, buf);
    lv_obj_set_style_text_color(lbl_wear_t, wearColor(truck.wear), 0);

    snprintf(buf, sizeof(buf), "Tr:%d%%", trailer.wear);
    lv_label_set_text(lbl_wear_tr, buf);
    lv_obj_set_style_text_color(lbl_wear_tr, wearColor(trailer.wear), 0);

    snprintf(buf, sizeof(buf), "C:%d%%",  trailer.cargoWear);
    lv_label_set_text(lbl_wear_c, buf);
    lv_obj_set_style_text_color(lbl_wear_c, wearColor(trailer.cargoWear), 0);

    // ── Trailer ───────────────────────────────────────────────
    if (trailer.attached && trailer.name.length() > 0) {
        int massT = trailer.mass / 1000;
        snprintf(buf, sizeof(buf), "%s (%dT)",
                 trailer.name.substring(0, 22).c_str(), massT);
        lv_label_set_text(lbl_trailer, buf);
    } else {
        lv_label_set_text(lbl_trailer, "--------Không có hàng hoá ------");
    }

    // ── Time remaining ────────────────────────────────────────
    if (job.remainingTime.length() > 0) {
        snprintf(buf, sizeof(buf), "Time: %s", job.remainingTime.c_str());
        lv_label_set_text(lbl_time, buf);
        lv_obj_set_style_text_color(lbl_time,
            job.remainingTime[0] == '-'
            ? lv_color_hex(0xFF4400) : lv_color_hex(0xFFFFFF), 0);
    }

    // ── Income ────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "$:%d", job.income);
    lv_label_set_text(lbl_income, buf);
}

// ════════════════════════════════════════════════════════
// 7. setupDisplay — gọi từ setup() trong main.cpp
// ════════════════════════════════════════════════════════
void setupDisplay() {
    // ── TFT_eSPI init ─────────────────────────────────────────
    tft.init();
    tft.setRotation(2);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif

    Serial.println("[TFT] Init OK");
    Serial.println("[PSRAM] Free: " + String(ESP.getFreePsram()));

/*     // ── Cấp phát LVGL buffer từ PSRAM ────────────────────────
    lvgl_buf1 = (lv_color_t*)ps_malloc(BUF_SIZE * sizeof(lv_color_t));
    lvgl_buf2 = (lv_color_t*)ps_malloc(BUF_SIZE * sizeof(lv_color_t));

    if (!lvgl_buf1 || !lvgl_buf2) {
        // PSRAM không đủ → fallback sang RAM thường
        Serial.println("[LVGL] PSRAM alloc failed, using DRAM");
        static lv_color_t static_buf1[BUF_SIZE];
        static lv_color_t static_buf2[BUF_SIZE];
        lvgl_buf1 = static_buf1;
        lvgl_buf2 = static_buf2;
    } else {
        Serial.println("[LVGL] Buffers in PSRAM OK");
    } */

    // ── LVGL init ─────────────────────────────────────────────
    lv_init();
        lvgl_create_buffers();
   // lv_disp_draw_buf_init(&draw_buf, lvgl_buf1, lvgl_buf2, BUF_SIZE);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = 240;
    disp_drv.ver_res  = 320;
    disp_drv.flush_cb = flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    showBootSequence();
    Serial.println("[LVGL] Init OK");

    // ── Tạo UI ────────────────────────────────────────────────
    createUI();

    Serial.println("[TFT] Display + Boot sequence OK");
}

// ════════════════════════════════════════════════════════
// 8. DisplayTask  (core 1, priority 2)
// ════════════════════════════════════════════════════════
void DisplayTask(void* parameter) {
    Serial.println("[DisplayTask] Started on core " + String(xPortGetCoreID()));
    Serial.println("[DisplayTask] Free heap: " + String(ESP.getFreeHeap()));

    for (;;) {
        unsigned long now = millis();

        if (now - lastTFTUpdate >= TFT_UPDATE_INTERVAL) {
            lastTFTUpdate = now;

            // Đọc data có mutex
            if (xSemaphoreTake(dataAccessMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                refreshUI();
                xSemaphoreGive(dataAccessMutex);
            }

            // lv_task_handler() NGOÀI mutex — flush SPI không cần mutex
            lv_timer_handler();
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
