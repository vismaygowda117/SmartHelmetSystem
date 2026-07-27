#ifndef ALERTS_H
#define ALERTS_H

#include <Arduino.h>

void HandleAlerts(int accidentState,
    bool wrongDirection,bool drowsyState,
    bool alcoholDetected,int speedAlert,
    bool helmetState);
void ResetAlerts(void);

#endif