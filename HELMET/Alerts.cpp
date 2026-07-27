#include "Alerts.h"
#include "Sensors.h"
#include "AccidentDetection.h"

// ALERT BLINK STATES
static unsigned long alertLastToggle = 0;
static bool alertBlinkState = false;

// ALERT BLINK FUNCTION
static void Alert_Blink(unsigned long interval, bool vibration)
{
    unsigned long now = millis();

    GreenLED_off();

    if (now - alertLastToggle >= interval)
    {
        alertLastToggle = now;
        alertBlinkState = !alertBlinkState;
    }

    if (alertBlinkState)
    {
        RedLED_on();
        Buzzer_on();

        if (vibration)
            Vibration_on();
        else
            Vibration_off();
    }
    else
    {
        RedLED_off();
        Buzzer_off();
        Vibration_off();
    }
}

// RESET ALERTS
void ResetAlerts(void)
{
    alertBlinkState = false;
    RedLED_off();
    Buzzer_off();
    Vibration_off();

    GreenLED_on();
}

// HANDLE ALERTS
void HandleAlerts( int accidentState,bool wrongDirection,
    bool drowsyState,bool alcoholDetected,int speedAlert,
    bool helmetState)
{
    bool alertActive = false;

    // ACCIDENT
    if (accidentState == AD_STATE_ACCIDENT)
    {
        alertActive = true;
        Alert_Blink(100, true);
    }

    // WRONG DIRECTION
    else if (wrongDirection)
    {
        alertActive = true;
        Alert_Blink(150, true);
    }

    // DROWSY
    else if (drowsyState)
    {
        alertActive = true;
        Alert_Blink(250, true);
    }

    // ALCOHOL
    else if (alcoholDetected)
    {
        alertActive = true;
        Alert_Blink(700, false);
    }

    // OVER SPEED
    else if (speedAlert == 1)
    {
        alertActive = true;
        Alert_Blink(300, false);
    }

    // NEAR SPEED
    else if (speedAlert == 2)
    {
        alertActive = true;
        Alert_Blink(800, false);
    }

    // HELMET
    else if (!helmetState)
    {
        alertActive = true;
        Alert_Blink(400, true);
    }

    // NO ALERT
    if (!alertActive)
    {
        ResetAlerts();
    }
}