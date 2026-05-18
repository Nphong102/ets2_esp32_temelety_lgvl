#pragma once

void setupWiFi();
void fetchTruckData();
void WiFiTask(void* parameter);

String formatTimeHHMM(String datetime);
