#ifndef GPS_H
#define GPS_H

#include <Arduino.h>

/* ================= TIME STRUCT ================= */
struct Time {
    int hour;
    int minute;
    int second;
    bool valid;
};

/* ================= DATE STRUCT ================= */
struct Date {
    int year;
    int month;
    int day;
    bool valid;
};

/* ================= GPS DATA ================= */
struct GPSData {
    double latitude;
    double longitude;
    double speed;
    double heading;       // direction (degrees)
    bool headingValid;
    bool valid;            
    Time time;
    Date date;
};

/* ================= FUNCTIONS ================= */
void GPS_init();
void GPS_update();
GPSData GPS_getData();
double getGpsSpeed();
String getISTDateTimeString(const Date *utcDate, const Time *utcTime);

#endif