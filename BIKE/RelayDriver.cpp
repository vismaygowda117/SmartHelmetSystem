#include <Arduino.h>
#include "RelayDriver.h"

/* ============== CONFIG ======================= */
#define RELAY_PIN 2   // GPIO pin connected to relay module

/* Initialize relay GPIO and set default OFF state */
void Relay_Init()
{
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
}

/* Turn relay ON (activate output) */
void Relay_On()
{
    digitalWrite(RELAY_PIN, HIGH);
}

/* Turn relay OFF (deactivate output) */
void Relay_Off()
{
    digitalWrite(RELAY_PIN, LOW);
}