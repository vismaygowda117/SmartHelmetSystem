#include "gps.h"
#include <TimeLib.h>
#include <TinyGPS++.h>

/* ================= STATIC OBJECTS ================= */
static HardwareSerial gpsSerial(1);
static TinyGPSPlus gps;
static GPSData data;

/* ================= CONSTANTS ================= */
#define IST_OFFSET_SEC  (5 * 3600 + 30 * 60)

/* ================= TIME CONVERSION ================= */
bool convertUTCtoIST(const Date *UTCdate, const Time *UTCtime,
                     Date *ISTdate, Time *ISTtime)
{
    tmElements_t tm;
    tm.Year   = UTCdate->year - 1970;
    tm.Month  = UTCdate->month;
    tm.Day    = UTCdate->day;
    tm.Hour   = UTCtime->hour;
    tm.Minute = UTCtime->minute;
    tm.Second = UTCtime->second;
    time_t t = makeTime(tm);
    t += IST_OFFSET_SEC;
    breakTime(t, tm);
    ISTdate->year  = tm.Year + 1970;
    ISTdate->month = tm.Month;
    ISTdate->day   = tm.Day;
    ISTtime->hour   = tm.Hour;
    ISTtime->minute = tm.Minute;
    ISTtime->second = tm.Second;
    return true;
}

/* ================= INIT ================= */
void GPS_init()
{
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
}

/* ================= UPDATE ================= */
void GPS_update()
{
    while (gpsSerial.available())
    {
        gps.encode(gpsSerial.read());
    }
}

/* ================= GET DATA ================= */
GPSData GPS_getData()
{
    GPS_update();
    bool locValid  = gps.location.isValid();
    bool timeValid = gps.time.isValid();
    bool dateValid = gps.date.isValid();
    /* ================= TIME ================= */
    static int lastSecond = -1;
    if (timeValid)
    {
        int sec = gps.time.second();
        if (sec != lastSecond)
        {
            lastSecond = sec;
            data.time.hour   = gps.time.hour();
            data.time.minute = gps.time.minute();
            data.time.second = sec;
            data.time.valid  = true;
        }
    }
    else
    {
        data.time.valid = false;
    }

    /* ================= DATE ================= */
    if (dateValid)
    {
        data.date.year  = gps.date.year();
        data.date.month = gps.date.month();
        data.date.day   = gps.date.day();
        data.date.valid = true;
    }
    else
    {
        data.date.valid = false;
    }

    /* ================= LOCATION + SPEED + HEADING ================= */
    if (gps.location.isUpdated() && locValid)
    {
        data.latitude  = gps.location.lat();
        data.longitude = gps.location.lng();
        // Speed
        if (gps.speed.isValid())
            data.speed = gps.speed.kmph();
        else
            data.speed = 0;
        // Heading
        if (gps.course.isValid())
        {
            data.heading = gps.course.deg();
            data.headingValid = true;
        }
        else
        {
            data.headingValid = false;
        }
        data.valid = true;   
    }
    else if (!locValid)
    {
        data.valid = false;
    }
    return data;
}

/* ================= SPEED ================= */
double getGpsSpeed()
{
    if (data.valid)
        return data.speed;
    return 0;
}

/* ================= IST STRING ================= */
String getISTDateTimeString(const Date *utcDate, const Time *utcTime)
{
    if (!utcDate || !utcTime || !utcDate->valid || !utcTime->valid)
        return "Time: Not available";
    Date istDate;
    Time istTime;
    convertUTCtoIST(utcDate, utcTime, &istDate, &istTime);
    char buffer[30];
    sprintf(buffer, "%02d-%02d-%04d %02d:%02d:%02d",
            istDate.day,istDate.month,istDate.year,
            istTime.hour,istTime.minute,istTime.second);
    return String(buffer);
}