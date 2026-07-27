#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "RoadMonitor.h"

// ================= OVERPASS =================
const char* OVERPASS_SERVERS[] =
{
    // Usually fastest and most reliable
    "https://overpass.kumi.systems/api/interpreter",
    // Fast mirror
    "https://lz4.overpass-api.de/api/interpreter",
    // Good community instance
    "https://overpass.private.coffee/api/interpreter",
    // Official main server
    "https://overpass-api.de/api/interpreter",
    // Additional fallback mirror
    "https://z.overpass-api.de/api/interpreter"
};
const int OVERPASS_SERVER_COUNT =  sizeof(OVERPASS_SERVERS) /sizeof(OVERPASS_SERVERS[0]);
static const char* QUERY_PREFIX =
    "[out:json][timeout:4];"
    "way(around:50,";

static const char* QUERY_SUFFIX =
    ")[highway~"
    "\"motorway|motorway_link|"
    "trunk|trunk_link|"
    "primary|primary_link|"
    "secondary|secondary_link|"
    "tertiary|tertiary_link|"
    "unclassified|residential|"
    "living_street|service\"]"
    ";out tags geom qt;";

// ================= GLOBAL STATUS =================
static RoadStatus currentRoadStatus;

// ================= QUERY TRACKING =================
static float lastQueryLat = 0;
static float lastQueryLon = 0;
static float lastQueryHeading = 0;
static bool firstRoadQuery = true;
static DynamicJsonDocument doc(60000);

// ================= DEFAULT SPEEDS =================
static int defaultSpeed(const String& roadType)
{
    if (roadType == "motorway") return 100;
    if (roadType == "motorway_link") return 80;
    if (roadType == "trunk") return 80;
    if (roadType == "trunk_link") return 70;
    if (roadType == "primary") return 60;
    if (roadType == "primary_link") return 55;
    if (roadType == "secondary") return 50;
    if (roadType == "secondary_link") return 45;
    if (roadType == "tertiary") return 40;
    if (roadType == "tertiary_link") return 35;
    if (roadType == "unclassified") return 35;
    if (roadType == "residential") return 30;
    if (roadType == "service") return 20;
    if (roadType == "living_street") return 15;
    return 40;
}

static String buildQuery(const GPSData& gps)
{
    String query;
    query.reserve(350);
    query += QUERY_PREFIX;
    query += String(gps.latitude, 6);
    query += ",";
    query += String(gps.longitude, 6);
    query += QUERY_SUFFIX;
    return query;
}

// ================= SPEED PARSER =================
static int parseMaxSpeed(String speedStr)
{
    speedStr.trim();
    speedStr.toLowerCase();
    speedStr.replace("km/h", "");
    speedStr.replace("kmh", "");
    speedStr.replace("kph", "");
    int semicolonIndex = speedStr.indexOf(';');
    if (semicolonIndex > 0)
    {
        speedStr = speedStr.substring(0, semicolonIndex);
    }
    String number = "";
    for (int i = 0; i < speedStr.length(); i++)
    {
        if (isDigit(speedStr[i]))
        {
            number += speedStr[i];
        }
        else if (number.length() > 0)
        {
            break;
        }
    }
    if (number.length() == 0)
    {
        return 0;
    }
    float speed = number.toFloat();
    if (speedStr.indexOf("mph") >= 0)
    {
        speed = speed * 1.60934f;
    }
    return (int)(speed + 0.5f);
}

// ================= DISTANCE =================
static float distanceMeters(float lat1,float lon1,float lat2,float lon2)
{
    float dLat = (lat2 - lat1) * PI / 180.0;
    float dLon = (lon2 - lon1) * PI / 180.0;
    float a = sin(dLat / 2) * sin(dLat / 2) + cos(lat1 * PI / 180.0) * 
            cos(lat2 * PI / 180.0) * sin(dLon / 2) * sin(dLon / 2);
    float c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return 6371000.0 * c;
}

// ================= HEADING DIFFERENCE =================
static float headingDifference(float a,float b)
{
    float diff = fabs(a - b);
    if (diff > 180)
    {
        diff = 360 - diff;
    }
    return diff;
}

// ================= BEARING =================
static float calculateBearing(float lat1,float lon1,float lat2,float lon2)
{
    float dLon = (lon2 - lon1) * PI / 180.0;
    lat1 *= PI / 180.0;
    lat2 *= PI / 180.0;
    float y = sin(dLon) * cos(lat2);
    float x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
    float bearing = atan2(y, x) * 180.0 / PI;
    bearing = fmod((bearing + 360.0), 360.0);
    return bearing;
}

// ================= LOCAL XY =================
static void latLonToXY(float refLat,float refLon,float lat,float lon,float& x,float& y)
{
    float latRad = refLat * PI / 180.0;
    x = (lon - refLon) * 111320.0 * cos(latRad);
    y = (lat - refLat) * 111320.0;
}

// ================= POINT TO SEGMENT DISTANCE =================
static float pointToSegmentDistance(float px,float py,float x1,float y1,float x2,float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float lenSq = dx * dx + dy * dy;
    if (lenSq < 0.0001)
    {
        return sqrt((px - x1) * (px - x1) + (py - y1) * (py - y1));
    }
    float t = ((px - x1) * dx + (py - y1) * dy) / lenSq;
    t = constrain(t, 0.0, 1.0);
    float projX = x1 + t * dx;
    float projY = y1 + t * dy;
    return sqrt((px - projX) * (px - projX) + (py - projY) * (py - projY));
}

// ================= SHOULD QUERY =================
static bool shouldQueryRoad(const GPSData& gps)
{
    if (!gps.valid)
    {
        return false;
    }
    if (firstRoadQuery)
    {
        return true;
    }
    float movedDistance = distanceMeters(lastQueryLat,lastQueryLon,gps.latitude,gps.longitude);
    float headingChange = headingDifference(lastQueryHeading,gps.heading);
    if (gps.speed < 8.0)
    {
        headingChange = 0;
    }
    if (movedDistance >= 60.0 || headingChange >= 35.0)
    {
        return true;
    }
    return false;
}

// ================= HTTP SETUP =================
static void setupHttp( HTTPClient& http, const char* server)
{
    http.setReuse(false);
    http.setTimeout(5000);
    http.begin(server);
    http.addHeader("Content-Type","application/x-www-form-urlencoded");
    http.addHeader("User-Agent", "ESP32 Smart Helmet" );
    http.addHeader("Connection","close");
}

// ================= HTTP =================
static bool sendOverpassRequest(const char* server, const String& query, String& payload)
{
    HTTPClient http;
    setupHttp(http, server);
    Serial.print("TRY SERVER: ");
    Serial.println(server);
    int httpCode = http.POST("data=" + query);
    bool success = false;
    if (httpCode >= 200 && httpCode < 300)
    {
        payload = http.getString();
        if (payload.startsWith("{"))
        {
            success = true;
        }
        else
        {
            Serial.println("INVALID JSON");
        }
    }
    else
    {
        Serial.print("HTTP ERROR: ");

        Serial.println(httpCode);
    }
    http.end();
    return success;
}
// ================= JSON =================
static bool parseRoadJson(const String& payload,DynamicJsonDocument& doc)
{
    DeserializationError err = deserializeJson(doc,payload);
    if (err)
    {
        Serial.println("JSON PARSE FAILED");
        return false;
    }
    return true;
}

// ================= SEGMENT MATCH =================
static bool findBestSegment(const GPSData& gps,JsonArray geometry,float& bestDistance,float& bestBearing)
{
    if (geometry.size() < 2)
    {
        return false;
    }
    bestDistance = 999999.0;
    bool found = false;
    float px = 0;
    float py = 0;
    for (int i = 0; i < geometry.size() - 1; i++)
    {
        JsonObject p1 = geometry[i];
        JsonObject p2 = geometry[i + 1];
        float lat1 = p1["lat"];
        float lon1 = p1["lon"];
        float lat2 = p2["lat"];
        float lon2 = p2["lon"];
        float segLength = distanceMeters(lat1,lon1,lat2,lon2);
        if (segLength < 1.0)
        {
            continue;
        }
        float x1, y1;
        float x2, y2;
        latLonToXY(gps.latitude,gps.longitude,lat1,lon1,x1,y1);
        latLonToXY(gps.latitude,gps.longitude,lat2,lon2,x2,y2);
        float dist = pointToSegmentDistance(px,py,x1,y1,x2,y2);
        if (dist < bestDistance)
        {
            bestDistance = dist;
            bestBearing = calculateBearing(lat1,lon1,lat2,lon2);
            found = true;
        }
    }
    return found;
}
// ================= FIND ROAD =================

// ================= FIND ROAD =================

static int findNearestRoad(const GPSData& gps,JsonArray elements,float& roadBearing)
{
    float bestDistance = 999999.0;
    int bestIndex = -1;
    int index = 0;
    for (JsonObject road : elements)
    {
        if (!road.containsKey("tags"))
        {
            index++;
            continue;
        }
        if (!road.containsKey("geometry"))
        {
            index++;
            continue;
        }
        JsonObject tags = road["tags"];
        if (!tags.containsKey("highway"))
        {
            index++;
            continue;
        }
        JsonArray geometry = road["geometry"];
        float roadDistance;
        float bearing;
        bool found = findBestSegment(gps,geometry,roadDistance,bearing);
        if (!found)
        {
            index++;
            continue;
        }
        // ================= BEARING FILTER =================
        if (gps.headingValid && gps.speed > 8.0)
        {
            float headingDiff = headingDifference(gps.heading, bearing);
            // Ignore roads with very different direction
            if (headingDiff > 90.0)
            {
                index++;
                continue;
            }
        }
        // ================= DISTANCE MATCH =================
        if (roadDistance < bestDistance)
        {
            bestDistance = roadDistance;
            roadBearing = bearing;
            bestIndex = index;
        }
        index++;
    }
    if (bestDistance > 150.0)
    {
        Serial.println("ROAD TOO FAR");
        return -1;
    }
    return bestIndex;
}
// ================= SPEED =================
static void updateRoadSpeed(JsonObject bestTags)
{
    currentRoadStatus.roadType = bestTags["highway"] | "";
    if (bestTags.containsKey("name"))
    {
        currentRoadStatus.roadName = bestTags["name"].as<String>();
    }
    else
    {
        currentRoadStatus.roadName = "UNKNOWN";
    }
    String oneway = bestTags["oneway"] | "no";
    currentRoadStatus.isOneway =  (oneway == "yes" ||  oneway == "1" ||  oneway == "true" ||
                                    oneway == "-1");
    if (bestTags.containsKey("maxspeed"))
    {
        String speedStr = bestTags["maxspeed"].as<String>();
        int parsedSpeed = parseMaxSpeed(speedStr);
        if (parsedSpeed > 0)
        {
            currentRoadStatus.maxSpeed = parsedSpeed;
        }
        else
        {
            currentRoadStatus.maxSpeed = defaultSpeed(currentRoadStatus.roadType);
        }
    }
    else
    {
        currentRoadStatus.maxSpeed =defaultSpeed(currentRoadStatus.roadType);
    }
}


// ================= SPEED ALERT =================
static void updateSpeedAlert(float speedKmh)
{
    static int lastAlert = 0;
    if (currentRoadStatus.maxSpeed <= 0)
    {
        currentRoadStatus.speedAlert = 0;
        return;
    }
    int limit = currentRoadStatus.maxSpeed;
    // OVER SPEED
    if (speedKmh >= limit + 2)
    {
        currentRoadStatus.speedAlert = 1;
    }
    // NEAR LIMIT
    else if (speedKmh >= limit * 0.80f)
    {
        currentRoadStatus.speedAlert = 2;
    }
    // SAFE
    else if (speedKmh <= limit * 0.75f)
    {
        currentRoadStatus.speedAlert = 0;
    }
    // HYSTERESIS
    else
    {
        currentRoadStatus.speedAlert = lastAlert;
    }
    lastAlert = currentRoadStatus.speedAlert;
}

// ================= WRONG DIRECTION =================
static void updateWrongDirection(const GPSData& gps,JsonObject bestTags,float roadBearing)
{
    currentRoadStatus.wrongDirection = false;
    if (!gps.headingValid)
    {
        return;
    }
    if (gps.speed < 10.0)
    {
        return;
    }
    String oneway = bestTags["oneway"] | "no";
    if (oneway == "yes" || oneway == "1" || oneway == "true")
    {
        float diff = headingDifference(gps.heading,roadBearing);
        if (diff > 140)
        {
            currentRoadStatus.wrongDirection = true;
            Serial.println("WRONG DIRECTION ALERT");
        }
    }
    else if (oneway == "-1")
    {
        float reverseBearing = fmod(roadBearing + 180.0,360.0);
        float diff = headingDifference(gps.heading,reverseBearing);
        if (diff > 140)
        {
            currentRoadStatus.wrongDirection = true;
            Serial.println("WRONG DIRECTION ALERT");
        }
    }
}

// ================= MAIN =================

bool RoadMonitor_update(const GPSData& gps,int retry)
{
    if (!gps.valid)
    {
        return false;
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }
    if (!shouldQueryRoad(gps))
    {
        return false;
    }
    String query = buildQuery(gps);
    String payload;
    payload.reserve(25000);
    bool requestOk = sendOverpassRequest(OVERPASS_SERVERS[retry],query,payload);
    if (!requestOk)
    {
        return false;
    }
    // Clears previous JSON data from reused document buffer
    // before parsing new Overpass response.
    doc.clear();
    if (!parseRoadJson(payload, doc))
    {
        return false;
    }
    JsonArray elements = doc["elements"];
    if (elements.size() == 0)
    {
        Serial.println("NO ROADS FOUND");
        return false;
    }
    float roadBearing = 0;
    int bestIndex = findNearestRoad(gps,elements,roadBearing);
    if (bestIndex < 0)
    {
        Serial.println("NO MATCHED ROAD");
        return false;
    }
    JsonObject bestRoad = elements[bestIndex];
    JsonObject bestTags = bestRoad["tags"];
    currentRoadStatus.matched = true;
    if (firstRoadQuery)
    {
        firstRoadQuery = false;
    }
    lastQueryLat = gps.latitude;
    lastQueryLon = gps.longitude;
    lastQueryHeading = gps.heading;
    updateRoadSpeed(bestTags);
    updateSpeedAlert(gps.speed);
    updateWrongDirection(gps,bestTags,roadBearing);
    return true;
}
// ================= GET STATUS =================
RoadStatus RoadMonitor_getStatus()
{
    return currentRoadStatus;
}