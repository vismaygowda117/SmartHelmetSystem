#include "Sensors.h"
#include <Arduino.h>

// ---------------- PINS ----------------
#define DROWSY_PIN 6
#define HELMET_PIN 5
#define MQ3_DO_PIN 4

#define BUZZER_PIN 18
#define VIBRATION_PIN 38

// Added
#define RED_LED_PIN 48
#define GREEN_LED_PIN 47


// ---------------- CONFIG ----------------
#define DROWSY_STABLE_TIME 1000 // 1 sec
#define HELMET_STABLE_TIME 300 // 1 sec
#define ALCOHOL_STABLE_TIME 300 // 300 ms

// ---------------- STATE ----------------
static unsigned long drowsyTimer = 0;
static bool drowsyStable = false;

static unsigned long helmetTimer = 0;
static bool helmetStable = false;

static unsigned long alcoholTimer = 0;
static bool alcoholDetected = false;

// ---------------- RAW READ ----------------
static bool Read_drowsy_raw();
static bool Read_helmet_raw();
static bool Read_alcohol_raw();


// ---------------- OUTPUT CONTROL ----------------
void Buzzer_on()
{
    digitalWrite(BUZZER_PIN, HIGH);
}

void Buzzer_off()
{
    digitalWrite(BUZZER_PIN, LOW);
}

void Vibration_on()
{
    digitalWrite(VIBRATION_PIN, HIGH);
}

void Vibration_off()
{
    digitalWrite(VIBRATION_PIN, LOW);
}

void waitForMQ3Stabilization(uint32_t timeoutMs)
{
    unsigned long startTime = millis();
    unsigned long stableStart = 0;

    Serial.println("MQ3 warming up...");

    while (millis() - startTime < timeoutMs)
    {
        Buzzer_off();

        bool state = Read_alcohol_raw();
        Serial.println(state);

        // HIGH = no alcohol
        if (state == LOW)
        {
            if (stableStart == 0)
            {
                stableStart = millis();
            }

            // must remain stable for 3 sec
            if (millis() - stableStart >= 3000)
            {
                Serial.print("MQ3 stabilized in: ");
                Serial.print(millis() - startTime);
                Serial.println(" ms");

                alcoholDetected = false;
                return;
            }
        }
        else
        {
            stableStart = 0;
        }

        delay(100);
    }

    Serial.println("MQ3 stabilization timeout");

    // fail safe
    alcoholDetected = false;
}
// ---------------- INIT ----------------
void Sensors_init()
{
    pinMode(DROWSY_PIN, INPUT);
    pinMode(HELMET_PIN, INPUT);
    pinMode(MQ3_DO_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(VIBRATION_PIN, OUTPUT);
    // Added LED pinMode
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    Buzzer_off();
    Vibration_off();
    unsigned long now = millis();
    drowsyTimer = now;
    helmetTimer = now;
    alcoholTimer = now;
    drowsyStable = Read_drowsy_raw();
    helmetStable = Read_helmet_raw();
    waitForMQ3Stabilization(30000);
    alcoholDetected = false;//Read_alcohol_raw();
}

// ---------------- RAW READ ----------------
static bool Read_drowsy_raw()
{
    return digitalRead(DROWSY_PIN) == LOW;
}

static bool Read_helmet_raw()
{
    return digitalRead(HELMET_PIN);
}

// Alcohol detected = 1
static bool Read_alcohol_raw()
{
    return digitalRead(MQ3_DO_PIN) == LOW;
}

// ---------------- DROWSY UPDATE ----------------
void Drowsy_update()
{
    static bool lastRaw = false;

    unsigned long now = millis();
    bool raw = Read_drowsy_raw();

    // Raw state changed
    if (raw != lastRaw)
    {
        lastRaw = raw;
        drowsyTimer = now;
    }

    // State stable long enough
    if (now - drowsyTimer >= DROWSY_STABLE_TIME)
    {
        drowsyStable = raw;
    }
}

// ---------------- HELMET UPDATE ----------------
void Helmet_update()
{
    static bool lastRaw = false;
    bool raw = Read_helmet_raw();
    // raw state changed
    if (raw != lastRaw)
    {
        lastRaw = raw;
        helmetTimer = millis();
    }
    // accept change only if stable long enough
    if (millis() - helmetTimer >= HELMET_STABLE_TIME)
    {
        helmetStable = raw;
    }
}

// ---------------- ALCOHOL UPDATE ----------------
void Alcohol_update()
{
    static bool lastRaw = false;
    bool raw = Read_alcohol_raw();
    // detect raw change
    if (raw != lastRaw)
    {
        lastRaw = raw;
        alcoholTimer = millis();
    }
    // accept only if stable
    if (millis() - alcoholTimer >= ALCOHOL_STABLE_TIME)
    {
        alcoholDetected = raw;
    }
}

// ---------------- GET ----------------
bool Drowsy_is_detected()
{
    return drowsyStable;
}

bool Helmet_is_worn()
{
    return helmetStable;
}

bool Alcohol_is_detected()
{
    return alcoholDetected;
}

// ---------------- LED CONTROL ----------------
void RedLED_on()
{
    digitalWrite(RED_LED_PIN, HIGH);
}

void RedLED_off()
{
    digitalWrite(RED_LED_PIN, LOW);
}

void GreenLED_on()
{
    digitalWrite(GREEN_LED_PIN, HIGH);
}

void GreenLED_off()
{
    digitalWrite(GREEN_LED_PIN, LOW);
}