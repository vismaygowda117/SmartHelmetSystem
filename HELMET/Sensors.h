#ifndef SENSORS_H
#define SENSORS_H

// -------- INIT --------
void Sensors_init();

// -------- SENSOR UPDATE --------
void Drowsy_update();
void Helmet_update();
void Alcohol_update();

// -------- SENSOR GET --------
bool Drowsy_is_detected();
bool Helmet_is_worn();
bool Alcohol_is_detected();

// -------- OUTPUT CONTROL --------
void Buzzer_on();
void Buzzer_off();

void Vibration_on();
void Vibration_off();

// -------- LED CONTROL --------
void RedLED_on();
void RedLED_off();

void GreenLED_on();
void GreenLED_off();

#endif