#pragma once

// ==================== Libraries ====================
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

// ==================== WiFi / Network ====================
extern char        WIFI_SSID[];
extern const char* WIFI_PASSWORD;
extern const char* TRUCK_DATA_URL;

// ==================== Timing Intervals (ms) ====================
#define WIFI_UPDATE_INTERVAL   200
#define TFT_UPDATE_INTERVAL    10    // LVGL cần gọi thường xuyên

// ==================== FreeRTOS ====================
extern TaskHandle_t     WiFiTaskHandle;
extern TaskHandle_t     DisplayTaskHandle;
extern SemaphoreHandle_t dataAccessMutex;

// ==================== Global TFT object ====================
extern TFT_eSPI tft;

// ==================== Timing ====================
extern unsigned long lastWiFiUpdate;
extern unsigned long lastTFTUpdate;
extern char          timeStr[9];
#define BAT_ADC_PIN  9