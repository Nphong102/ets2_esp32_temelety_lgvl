/**
 * main.cpp — ETS2 Dashboard (minimal: WiFi + LVGL TFT)
 *
 * Core 0: WiFiTask  — fetch telemetry mỗi 200ms
 * Core 1: DisplayTask — LVGL render mỗi 10ms
 */

#include "config.h"
#include "data_types.h"
#include "wifi_task.h"
#include "display_task.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=============================");
    Serial.println(" ETS2 Dashboard — Booting...");
    Serial.println("=============================");
    Serial.println("Free heap:  " + String(ESP.getFreeHeap()));
    Serial.println("Free PSRAM: " + String(ESP.getFreePsram()));
    pinMode(BAT_ADC_PIN, INPUT);
    // 1. Mutex — tạo trước tất cả
    dataAccessMutex = xSemaphoreCreateMutex();
    if (!dataAccessMutex) {
        Serial.println("[ERROR] Mutex create failed!");
        while (1);
    }
    Serial.println("[OK] Mutex created");

    // 2. TFT + LVGL — phải trước khi tạo DisplayTask
    setupDisplay();
    Serial.println("[OK] Display setup done");

    // 3. WiFi
    setupWiFi();
    Serial.println("[OK] WiFi setup done");

    // 4. Tạo tasks
    //    DisplayTask: core 1, priority 2, stack 6KB
    //    (buffer trong PSRAM nên stack task nhỏ cũng được)
    xTaskCreatePinnedToCore(
        DisplayTask, "Display",
        10240,                   // 6KB stack
        NULL, 2,
        &DisplayTaskHandle, 1
    );
    Serial.println("[OK] DisplayTask created");

    //    WiFiTask: core 0, priority 1, stack 8KB
    xTaskCreatePinnedToCore(
        WiFiTask, "WiFi",
        8192,                   // 8KB stack
        NULL, 1,
        &WiFiTaskHandle, 0
    );
    Serial.println("[OK] WiFiTask created");

    Serial.println("[OK] All done — entering loop()");
}

void loop() {
    // Tất cả logic trong FreeRTOS tasks
    // loop() chỉ in heap mỗi 10s để debug
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 10000) {
        lastPrint = millis();
        Serial.printf("[heap] free=%d  psram=%d\n",
                      ESP.getFreeHeap(), ESP.getFreePsram());
        Serial.printf("DisplayTask stack free: %d\n", uxTaskGetStackHighWaterMark(DisplayTaskHandle));

    }
    vTaskDelay(pdMS_TO_TICKS(1000));
}
