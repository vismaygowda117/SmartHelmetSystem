#ifndef ROADMONITOR_H
#define ROADMONITOR_H

#include <Arduino.h>
#include "GPS.h"

struct RoadStatus
{
    String roadType;
    String roadName;
    int maxSpeed;
    bool matched;
    bool isOneway;
    bool wrongDirection;
    int speedAlert;
};
extern const int OVERPASS_SERVER_COUNT;
bool RoadMonitor_update(const GPSData& gps,int retry);
RoadStatus RoadMonitor_getStatus();
#endif