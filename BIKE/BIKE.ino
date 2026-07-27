#include <Arduino.h>
#include "ble_client.h"
#include "RelayDriver.h"
#include "Display.h"
#include "Alerts.h"


extern double lat;
extern double lon ;
extern double speed ;
extern double maxSpeed ;
extern int speedAlert ;
extern int alcoholAlert ;
extern int drowsyAlert;
extern int helmetWearAlert;
extern int ignitionStatus;
extern int driverMobileState;
extern int directionAlert;
extern int AccidentState;
extern char gpsTime[50];
unsigned long lastDisplayUpdate = 0;
extern bool BLEconnected;


void setup()
{
    Serial.begin(115200);
    delay(1000);
    while (!Serial);

    Serial.println("Bike Unit Start");
    Display_init();
    ble_client_init();
    Relay_Init();
    Alert_init();
}

void loop()
{
    ble_client_loop();
    static bool lastState = false;
    bool ignition = false;
    if (IsHelmetDataValid())
    {
        ignition = ignitionStatus;
    }
    if (ignition != lastState)
    {
        lastState = ignition;
        if (ignition)
        {
            Relay_On();
        }
        else
        {
            Relay_Off();
        }
    }
    if (IsHelmetDataValid())
    {
        Alert_update(helmetWearAlert, alcoholAlert, drowsyAlert, speedAlert, directionAlert);
    }
    else
    {
        resetHelmetData();
        ResetAlerts();
    }

    if (millis() - lastDisplayUpdate >= 1000)
    {
        lastDisplayUpdate = millis();
        Display_update(BLEconnected, driverMobileState,ignitionStatus, gpsTime,speed, maxSpeed);
        PrintHelmetDebug();
    }
}


