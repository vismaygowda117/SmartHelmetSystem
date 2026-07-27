#pragma once

#include <Arduino.h>

// ---------------- PUBLIC API ----------------

// Initialize BLE server
void BLE_init();
// Check connection status
bool ble_isConnected();
// Send data over BLE
void ble_process(double lat, double lon, double speed, float maxSpeed,
                 int speedAlert, int alcohol, int drowsy,
                 int helmetWear,
                 int ignition, int driverMobile,
                 const char* gpsDateTime,
                 int direction, int accidentState);