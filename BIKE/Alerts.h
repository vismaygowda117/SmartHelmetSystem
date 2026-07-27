#ifndef ALERT_H
#define ALERT_H

#include <Arduino.h>

// ================= FUNCTIONS =================
void Alert_init();
void Alert_update(int helmetWearAlert, int alcoholAlert, int drowsyAlert, int speedAlert, int directionAlert);
void ResetAlerts();
#endif