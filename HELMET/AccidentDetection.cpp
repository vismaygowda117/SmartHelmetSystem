#include "AccidentDetection.h"
#include "GPS.h"
#include <Wire.h>
#include <math.h>
#include <MPU6050_tockn.h>

/* ============================ CONSTANTS ============================ */
#define ALERT_COOLDOWN          10000
#define RECOVERY_STABLE_TIME    1000

#define GYRO_THRESHOLD          350.0f
#define TILT_ANGLE_THRESHOLD    75.0f
#define IMPACT_THRESHOLD        2.2f
#define SPEED_CHANGE_THRESHOLD  25.0f

/* ============================= OBJECTS ============================= */
MPU6050 mpu(Wire);

/* ========================= STATE VARIABLES ========================= */
static float prevSpeed = 0.0f;
static uint32_t lastAlertTime = 0;
static uint32_t recoveryStart = 0;
static AD_State currentState = AD_STATE_NORMAL;

/* ============================ INIT ============================ */
void AD_Init(void)
{
    Wire.begin(8,9);
    Serial.print("Initializing MPU...");
    mpu.begin();
    mpu.calcGyroOffsets(true);
    Serial.println("\nMPU6050 ready!");
}

/* ============================ UPDATE ============================ */
void AD_Update(void)
{
    mpu.update();

    /* ================= GYRO VALUES ================= */
    float gx = mpu.getGyroX();
    float gy = mpu.getGyroY();
    float gz = mpu.getGyroZ();

    float gyroMagnitude = sqrtf(gx * gx + gy * gy + gz * gz);

    /* ================= ACCEL VALUES ================= */
    float ax = mpu.getAccX();
    float ay = mpu.getAccY();
    float az = mpu.getAccZ();

    /* Total acceleration magnitude */
    float totalAccel = sqrtf(ax * ax + ay * ay + az * az);

    /* Remove gravity (~1g) */
    float impactForce = fabs(totalAccel - 1.0f);

    /* ================= TILT ================= */
    float tiltAngle = atan2f(ay, az) * 180.0f / PI;

    /* ================= SPEED ================= */
    float currentSpeed = getGpsSpeed();
    float speedDelta = fabsf(prevSpeed - currentSpeed);

    uint32_t now = millis();

    /* ================= ACCIDENT LOGIC ================= */

    bool severeTilt =
        fabs(tiltAngle) > TILT_ANGLE_THRESHOLD;

    bool severeRotation =
        gyroMagnitude > GYRO_THRESHOLD;

    bool strongImpact =
        impactForce > IMPACT_THRESHOLD;

    bool suddenSpeedDrop =
        (prevSpeed > 20.0f) &&
        (speedDelta > SPEED_CHANGE_THRESHOLD);

    /*
        Accident only if:
        1. Strong impact + tilt
        OR
        2. Strong rotation + impact
        OR
        3. Sudden speed drop + impact
    */

    bool accidentCondition =
        (strongImpact && severeTilt) ||
        (strongImpact && severeRotation) ||
        (strongImpact && suddenSpeedDrop);

    /* ================= STATE MACHINE ================= */

    if (accidentCondition && currentState == AD_STATE_NORMAL)
    {
        currentState = AD_STATE_ACCIDENT;
        lastAlertTime = now;
        recoveryStart = 0;

        Serial.println("ACCIDENT DETECTED");
    }

    if (currentState == AD_STATE_ACCIDENT)
    {
        if (!accidentCondition)
        {
            if (recoveryStart == 0)
                recoveryStart = now;

            if ((now - recoveryStart > RECOVERY_STABLE_TIME) &&
                (now - lastAlertTime > ALERT_COOLDOWN))
            {
                currentState = AD_STATE_NORMAL;
                recoveryStart = 0;

                Serial.println("RECOVERED");
            }
        }
        else
        {
            recoveryStart = 0;
        }
    }

    prevSpeed = currentSpeed;
}

/* ============================ GET STATE ============================ */
AD_State AD_GetState(void)
{
    return currentState;
}