#include "Alerts.h"

// =====================================================
// ALERT STRUCTURE
// =====================================================

struct AlertLED
{
    int redPin;
    int greenPin;
    bool blinkState;
    unsigned long lastToggle;
};

// =====================================================
// ALERT OBJECTS
// =====================================================

// Helmet
AlertLED helmetAlert =
{
    3,      // RED
    4,      // GREEN
    false,
    0
};

// Alcohol
AlertLED alcoholAlertLed =
{
    1,      // RED 
    0,      // GREEN
    false, 
    0
};

// Drowsy
AlertLED drowsyAlertLed =
{
    7,      // RED
    5,      // GREEN
    false,
    0
};

AlertLED SpeedAlertLed =
{
    21,      // RED
    20,      // GREEN
    false,
    0
};
// Drowsy
AlertLED DirectionAlertLed =
{
    10,      // RED
    6,      // GREEN
    false,
    0
};


// =====================================================
// LED HELPERS
// =====================================================

// ACTIVE HIGH LEDs

void Red_on(AlertLED &alert)
{
    digitalWrite(alert.redPin, HIGH);
}

void Red_off(AlertLED &alert)
{
    digitalWrite(alert.redPin, LOW);
}

void Green_on(AlertLED &alert)
{
    digitalWrite(alert.greenPin, HIGH);
}

void Green_off(AlertLED &alert)
{
    digitalWrite(alert.greenPin, LOW);
}

// =====================================================
// INIT
// =====================================================

void Alert_init()
{
    // Helmet
    pinMode(helmetAlert.redPin, OUTPUT);
    pinMode(helmetAlert.greenPin, OUTPUT);
    // Alcohol
    pinMode(alcoholAlertLed.redPin, OUTPUT);
    pinMode(alcoholAlertLed.greenPin, OUTPUT);
    // Drowsy
    pinMode(drowsyAlertLed.redPin, OUTPUT);
    pinMode(drowsyAlertLed.greenPin, OUTPUT);
    //Speed alert
    pinMode(SpeedAlertLed.redPin, OUTPUT);
    pinMode(SpeedAlertLed.greenPin, OUTPUT);

    //Direction alert
    pinMode(DirectionAlertLed.redPin, OUTPUT);
    pinMode(DirectionAlertLed.greenPin, OUTPUT);

    // Initial OFF
    Red_off(helmetAlert);
    Green_off(helmetAlert);
    Red_off(alcoholAlertLed);
    Green_off(alcoholAlertLed);
    Red_off(drowsyAlertLed);
    Green_off(drowsyAlertLed);
    Red_off(SpeedAlertLed);
    Green_off(SpeedAlertLed);
    Red_off(DirectionAlertLed);
    Green_off(DirectionAlertLed);

}

// =====================================================
// GENERIC BLINK
// =====================================================

void Alert_Blink(AlertLED &alert, unsigned long interval)
{
    unsigned long now = millis();
    // GREEN OFF during alert
    Green_off(alert);
    // Toggle state
    if (now - alert.lastToggle >= interval)
    {
        alert.lastToggle = now;
        alert.blinkState = !alert.blinkState;
    }
    // RED control
    if (alert.blinkState)
    {
        Red_on(alert);
    }
    else
    {
        Red_off(alert);
    }
}

void Alert_YellowBlink(AlertLED &alert, unsigned long interval)
{
    unsigned long now = millis();

    if (now - alert.lastToggle >= interval)
    {
        alert.lastToggle = now;
        alert.blinkState = !alert.blinkState;
    }

    if (alert.blinkState)
    {
        // RED ON
        analogWrite(alert.redPin, 255);
        analogWrite(alert.greenPin, 0);
    }
    else
    {
        // GREEN ON
        analogWrite(alert.redPin, 0);
        analogWrite(alert.greenPin, 255);
    }
}
// NORMAL STATE
void Alert_Normal(AlertLED &alert)
{
    alert.blinkState = false;
    Red_off(alert);
    Green_on(alert);
}

void ResetAlerts()
{
    Red_off(helmetAlert);
    Green_off(helmetAlert);
    Red_off(alcoholAlertLed);
    Green_off(alcoholAlertLed);
    Red_off(drowsyAlertLed);
    Green_off(drowsyAlertLed);
    Red_off(SpeedAlertLed);
    Green_off(SpeedAlertLed);
    Red_off(DirectionAlertLed);
    Green_off(DirectionAlertLed);

}
// UPDATE
void Alert_update(int helmetWearAlert, int alcoholAlert, int drowsyAlert, 
                    int speedAlert, int directionAlert)
{
    // ------HELMET ALERT-------------------------------
    if (!helmetWearAlert)
    {
        Alert_Blink(helmetAlert, 400);
    }
    else
    {
        Alert_Normal(helmetAlert);
    }
    // ---------ALCOHOL ALERT---------------------------
    if (alcoholAlert)
    {
        Alert_Blink(alcoholAlertLed, 400);
    }
    else
    {
        Alert_Normal(alcoholAlertLed);
    }
    // DROWSY ALERT
    if (drowsyAlert)
    {
        Alert_Blink(drowsyAlertLed, 400);
    }
    else
    {
        Alert_Normal(drowsyAlertLed);
    }
    // SPEED ALERT - 
    if (speedAlert == 1)
    {
        Alert_Blink(SpeedAlertLed, 100);
    }
    else if (speedAlert == 2)  // Near Speed alert
    {
       //Alert_Blink(SpeedAlertLed, 750);  
       Alert_YellowBlink(SpeedAlertLed, 1000);
    }
    else
    {
        Alert_Normal(SpeedAlertLed);   // Normal speed
    }

     // DIrection ALERT
    if (directionAlert)
    {
        Alert_Blink(DirectionAlertLed, 400);
    }
    else
    {
        Alert_Normal(DirectionAlertLed);
    }


}