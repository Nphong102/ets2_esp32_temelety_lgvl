#pragma once
#include <Arduino.h>

// ==================== Truck Data ====================
struct TruckData {
  int    speed = 0;
  int    rpm = 0;
  int    fuel = 0;
  int    fuelCapacity = 0;
  bool   fuelWarningOn = false;
  bool   airWarningOn = false;
  int    airPressure = 0;
  int    displayedGear = 0;
  int    waterTemperature = 0;
  int    oilTemperature = 0;
  int    speedLimit = 0;
  int    odo = 0;
  bool   blinkerLeftOn = false;
  bool   blinkerRightOn = false;
  bool   parkingLight = false;
  bool   lightsBeamHighOn = false;
  bool   lightsBeamLowOn = false;
  bool   hazardWarningLights = false;
  bool   parkingBreak = false;
  bool   engineOn = false;
  bool   checkEngOn = false;
  int    wear = 0;
  int    speedCtrl = 0;
  bool   speedCtrlOn = false;
  String make = "";
  String model = "";
  String id = "";
  String licensePlate = "";
};

// ==================== Game Data ====================
struct GameData {
  String time = "";
  String realTime = "";
  bool   pause = true;
};

// ==================== Job Data ====================
struct JobData {
  String remainingTime = "";
  int    income = 0;
};

// ==================== Trailer Data ====================
struct TrailerData {
  bool   attached = false;
  String name = "";
  int    mass = 0;
  int    wear = 0;
  int    cargoWear = 0;
};

// ==================== Navigation Data ====================
struct NavigationData {
  int estimatedDistance = 0;
};

// ==================== Extern declarations ====================
extern TruckData      truck;
extern GameData       game;
extern JobData        job;
extern TrailerData    trailer;
extern NavigationData navigation;
