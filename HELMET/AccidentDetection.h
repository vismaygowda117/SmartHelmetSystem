#ifndef ACCIDENT_DETECTION_H
#define ACCIDENT_DETECTION_H

#include <Arduino.h>

/* ============================ STATE ENUM ============================ */
typedef enum
{
    AD_STATE_NORMAL = 0,
    AD_STATE_ACCIDENT
} AD_State;

/* ============================ FUNCTIONS ============================ */
void AD_Init(void);
void AD_Update(void);
AD_State AD_GetState(void);

#endif