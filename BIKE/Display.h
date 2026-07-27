// ======================================================
// Display.h
// ======================================================

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

void Display_init();

void Display_update(bool bluetoothConnected,
                    bool mobileConnected,
                    bool ignitionOn,
                    const char* gpsTime,
                    float speed,
                    float speedLimit);
#endif