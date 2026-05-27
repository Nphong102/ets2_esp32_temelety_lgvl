/**
 * wifi_task.cpp
 * - Kết nối WiFi
 * - Fetch JSON từ ETS2 telemetry API
 * - Chạy trong WiFiTask trên core 0
 */

#include "config.h"
#include "data_types.h"
#include "wifi_task.h"

// ════════════════════════════════════════════════════════
// setupWiFi
// ════════════════════════════════════════════════════════
void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("[WiFi] Connecting");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] Connected: " + WiFi.localIP().toString());
    } else {
        Serial.println("\n[WiFi] Failed — will retry in WiFiTask");
    }
}

// ════════════════════════════════════════════════════════
// formatTimeHHMM  "2024-01-01T14:35:00" → "14:35"
// ════════════════════════════════════════════════════════
String formatTimeHHMM(String datetime) {
    int h = 0, m = 0;
    sscanf(datetime.c_str(), "%*d-%*d-%*dT%d:%d", &h, &m);
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    return String(buf);
}

// ════════════════════════════════════════════════════════
// fetchTruckData
// ════════════════════════════════════════════════════════
void fetchTruckData() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    http.begin(TRUCK_DATA_URL);
    http.setTimeout(150);   // timeout ngắn để không block lâu
    int code = http.GET();

    if (code == 200) {
        String payload = http.getString();
        //    Serial.println(payload);   // uncomment nếu muốn xem raw JSON
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            Serial.println("[JSON] Error: " + String(err.c_str()));
            http.end();
            return;
        }

        // Lấy mutex trước khi ghi data
        if (xSemaphoreTake(dataAccessMutex, pdMS_TO_TICKS(50)) == pdTRUE) {

            // ── Truck ──────────────────────────────────────
            truck.speed            = abs((int)doc["truck"]["speed"]);
            truck.rpm              = doc["truck"]["engineRpm"];
            truck.fuel             = doc["truck"]["fuel"];
            truck.fuelCapacity     = doc["truck"]["fuelCapacity"];
            truck.fuelWarningOn    = doc["truck"]["fuelWarningOn"];
            truck.airPressure      = doc["truck"]["air"];
            truck.airWarningOn     = doc["truck"]["airWarningOn"];
            truck.displayedGear    = doc["truck"]["displayedGear"];
            truck.waterTemperature = doc["truck"]["watterTemp"];
            truck.oilTemperature   = doc["truck"]["oilTemp"];
            truck.speedLimit       = doc["navigation"]["speedLimit"];
            truck.odo              = doc["truck"]["odometer"];
            truck.wear             = (int)(doc["truck"]["maxWear"].as<float>() * 100);
            truck.engineOn         = doc["truck"]["engineOn"];
            truck.parkingBreak     = doc["truck"]["parkingBreak"];
            truck.lightsBeamHighOn = doc["truck"]["lightsBeamHighOn"];
            truck.lightsBeamLowOn  = doc["truck"]["lightsBeamLowOn"];
            truck.blinkerLeftOn    = doc["truck"]["blinkerLeftActive"];
            truck.blinkerRightOn   = doc["truck"]["blinkerRightActive"];
            truck.speedCtrl        = doc["truck"]["cruiseControlSpeed"];
            truck.speedCtrlOn      = doc["truck"]["cruiseControlOn"];
            truck.make             = doc["truck"]["make"].as<String>();
            truck.model            = doc["truck"]["model"].as<String>();
            truck.licensePlate     = doc["truck"]["licensePlate"].as<String>();
            truck.id               = doc["truck"]["id"].as<String>();

            // ── Game ───────────────────────────────────────
            game.time     = formatTimeHHMM(doc["game"]["time"].as<String>());
            game.realTime = doc["game"]["realTime"].as<String>();
            game.pause    = doc["game"]["paused"];

            // ── Job ────────────────────────────────────────
            job.remainingTime = doc["job"]["timeRemain"].as<String>();
            job.income        = doc["job"]["income"];

            // ── Trailer ────────────────────────────────────
            trailer.attached  = doc["trailer"]["attached"];
            trailer.name      = doc["trailer"]["name"].as<String>();
            trailer.mass      = doc["trailer"]["mass"];
            trailer.wear      = (int)(doc["trailer"]["trailerWear"].as<float>() * 100);
            trailer.cargoWear = (int)(doc["trailer"]["cargoWear"].as<float>()   * 100);
            // Serial.println(truck.speed);
            // ── Navigation ─────────────────────────────────
            navigation.estimatedDistance =
                doc["navigation"]["estimatedDistance"].as<int>() / 1000;
            navigation.speed_limit = round(doc["navigation"]["speedLimit"].as<int>()/5*5);

            xSemaphoreGive(dataAccessMutex);
        }
    } else {
        Serial.println("[HTTP] Code: " + String(code));
    }

    http.end();
}

// ════════════════════════════════════════════════════════
// WiFiTask  (core 0, priority 1)
// ════════════════════════════════════════════════════════
void WiFiTask(void* parameter) {
    Serial.println("[WiFiTask] Started on core " + String(xPortGetCoreID()));

    for (;;) {
        // Reconnect nếu mất mạng
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Reconnecting...");
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        unsigned long now = millis();
        if (now - lastWiFiUpdate >= WIFI_UPDATE_INTERVAL) {
            lastWiFiUpdate = now;
            fetchTruckData();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
