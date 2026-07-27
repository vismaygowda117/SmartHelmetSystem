// ======================================================
// Display.cpp
// ======================================================
#include "Display.h"
#include <Wire.h>
#include <cstdio>
#include <LiquidCrystal_AIP31068_I2C.h>
#define LCD_ADDR 0x3E

static LiquidCrystal_AIP31068_I2C lcd(LCD_ADDR, 20,4);
// ======================================================
// INIT
// ======================================================
void Display_init()
{
    Wire.begin(8, 9);
    lcd.begin(20, 4);
    lcd.setContrast(22);
    lcd.clear();
    // "Smart Helmet" = 12 chars
    // (20 - 12) / 2 = 4
    lcd.setCursor(4, 1);
    lcd.print("SMART HELMET");
    // "Initializing" = 12 chars
    lcd.setCursor(4, 2);
    lcd.print("INITIALIZING");
    delay(2000);
    lcd.clear();
}
// ======================================================
// UPDATE DISPLAY
// ======================================================
void Display_update(bool bluetoothConnected, bool mobileConnected,
                    bool ignitionOn, const char* gpsTime,
                    float speed, float speedLimit)
{
    char buffer[21];
    // ================= ROW 0 =================
    snprintf(buffer, sizeof(buffer), "%-20s", gpsTime);
    lcd.setCursor(0, 0);
    lcd.print(buffer);
    // ================= ROW 1 =================
    snprintf(buffer, sizeof(buffer),"BT:%s MB:%s IG:%s",
             bluetoothConnected ? "OK" : "NO",
             mobileConnected ? "OK" : "NO",
             ignitionOn ? "OK" : "NO");
    lcd.setCursor(0, 1);
    lcd.print("                    ");
    lcd.setCursor(0, 1);
    lcd.print(buffer);
    // ================= ROW 2 =================
    snprintf(buffer, sizeof(buffer), "SPEED    :%5.1fkm/h", speed);
    lcd.setCursor(0, 2);
    lcd.print("                    ");
    lcd.setCursor(0, 2);
    lcd.print(buffer);
    // ================= ROW 3 =================
    snprintf(buffer, sizeof(buffer),"SPD LIMIT:%5.1fkm/h", speedLimit);
    lcd.setCursor(0, 3);
    lcd.print("                    ");
    lcd.setCursor(0, 3);
    lcd.print(buffer);
}