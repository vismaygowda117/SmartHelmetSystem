#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gps.h"
#include "AccidentDetection.h"
#include "BLEComm.h"
#include "Sensors.h"
#include "RoadMonitor.h"
#include "Telegram.h"
#include "WiFiComm.h"
#include "Alerts.h"

/* ============================== TIMERS ============================== */
unsigned long lastGPS = 0;
unsigned long lastAccident = 0;
unsigned long lastPrint = 0;
unsigned long lastAlcohol = 0;
unsigned long lastBLE = 0;
unsigned long lastAlertCheck = 0;
unsigned long lastDrowsy = 0;
unsigned long lastHelmet = 0;
unsigned long lastWiFiRun = 0;
int ignition = 0;
TaskHandle_t RoadTaskHandle = NULL;
/* ============================ INTERVALS ============================ */
const unsigned long GPS_INTERVAL = 200;
const unsigned long ACCIDENT_INTERVAL = 50;
const unsigned long PRINT_INTERVAL = 1000;
const unsigned long ALCOHOL_INTERVAL = 100;
const unsigned long BLE_INTERVAL = 1000;
const unsigned long ALERT_INTERVAL = 1000;
const unsigned long DROWSY_INTERVAL = 100;
const unsigned long HELMET_INTERVAL = 100;
const unsigned long WIFI_INTERVAL = 5500;
/* ============================ SHARED DATA ============================ */
GPSData CurrentGpsData;
GPSData RoadTaskGpsData;
GPSData lastValidGPSData;
AD_State accidentState = AD_STATE_NORMAL;
RoadStatus RoadStatusData;
int maxSpeed = 0;
int speedAlert = 0;
bool wrongDirection = false;
portMUX_TYPE gpsMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE roadDataMux = portMUX_INITIALIZER_UNLOCKED;
// Critical section mutex
portMUX_TYPE statusMux = portMUX_INITIALIZER_UNLOCKED;
/* ============================ STATES ============================ */
bool alertSent = false;
bool drowsyState = false;
bool helmetState = false;
bool alcoholDetected = false;
/* ============================ GLOBAL STATUS ============================ */
String statusMsg ;
String TelegramstatusMsg ;
/* ============================ FUNCTION DECLARATION ============================ */
String buildMessage(GPSData current, GPSData lastValid,
                    bool isDrowsy, bool helmetState,
                    float maxAllowedSpeed,
                    bool alcoholDetected, bool driverMobile,
                    int speedAlert,
                    bool wrongDirection,
                    int accidentState);  
void sendAccidentTelegram(String msg);
/* ================= ROAD TASK ================= */
void RoadMonitorTask(void *pvParameters)
{
    while (true)
    {
        bool success = false;
        for (int retry = 0; retry < OVERPASS_SERVER_COUNT; retry++)
        {
            GPSData gpsCopy;
            portENTER_CRITICAL(&gpsMux);
            gpsCopy = RoadTaskGpsData;
            portEXIT_CRITICAL(&gpsMux);
            if (!gpsCopy.valid)
            {
                break;
            }
            success = RoadMonitor_update(gpsCopy, retry);
            if (success)
            {
                portENTER_CRITICAL(&roadDataMux);
                RoadStatusData = RoadMonitor_getStatus();
                portEXIT_CRITICAL(&roadDataMux);
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(20));       
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void TelegramTask(void *pvParameters)
{
    String localMsg;
    localMsg.reserve(1500);
    while (true)
    {
        if (WiFiComm_isConnected())
        {
            // Copy shared message safely
            portENTER_CRITICAL(&statusMux);
            localMsg = TelegramstatusMsg;
            portEXIT_CRITICAL(&statusMux);
            // Use copied message outside critical section
            telegram_loop(localMsg);
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* ============================ SETUP ============================ */
void setup()
{
    Serial.begin(115200);
    statusMsg.reserve(1500);
    TelegramstatusMsg.reserve(1500);
    GPS_init();
    AD_Init();
    BLE_init();
    Sensors_init();
    lastValidGPSData.valid = false;
    WiFiComm_init();
    telegram_init(); 
    /* ================= ROAD TASK ================= */
    xTaskCreatePinnedToCore(RoadMonitorTask, "RoadTask",
        10000, NULL,2,&RoadTaskHandle,0);
    /* ================= TELEGRAM TASK ================= */
    xTaskCreatePinnedToCore(TelegramTask,"TelegramTask",
        12000, NULL,1,NULL,1);
    Serial.println(F("System Ready"));
}

/* ============================ LOOP ============================ */
void loop()
{
     unsigned long now = millis();
     // -------- WiFi --------
    if (now - lastWiFiRun >= WIFI_INTERVAL)
    {
        lastWiFiRun = now;
        WiFiComm_loop();
    }
    // -------- Accident -------
    if (now - lastAccident >= ACCIDENT_INTERVAL)
    {
        lastAccident = now;
        AD_Update();
        accidentState = AD_GetState();
    }
    // -------- Drowsy --------
    if (now - lastDrowsy >= DROWSY_INTERVAL)
    {
        lastDrowsy += DROWSY_INTERVAL;
        Drowsy_update();
        drowsyState = Drowsy_is_detected();
    }
    // -------- Helmet --------
    if (now - lastHelmet >= HELMET_INTERVAL)
    {
        lastHelmet += HELMET_INTERVAL;
        Helmet_update();
        helmetState = Helmet_is_worn();
    }
    // -------- Alcohol --------
    if (now - lastAlcohol >= ALCOHOL_INTERVAL)
    {
        lastAlcohol = now;
        Alcohol_update();
        alcoholDetected = Alcohol_is_detected();
    }
    // -------- GPS --------
    if (now - lastGPS >= GPS_INTERVAL)
    {
        lastGPS = now;
        CurrentGpsData = GPS_getData();
        //share GPS data to Road task 
        portENTER_CRITICAL(&gpsMux);
        RoadTaskGpsData = CurrentGpsData;
        portEXIT_CRITICAL(&gpsMux);
        if (CurrentGpsData.valid)
            lastValidGPSData = CurrentGpsData;
    }
    portENTER_CRITICAL(&roadDataMux);
    maxSpeed = RoadStatusData.maxSpeed;
    speedAlert = RoadStatusData.speedAlert;
    wrongDirection = RoadStatusData.wrongDirection;
    portEXIT_CRITICAL(&roadDataMux);
    bool driverMobile = telegram_isWiFiConnected();
    /* ================= DEBUG ================= */
    if (now - lastPrint >= PRINT_INTERVAL)
    {
       lastPrint = now;
       Serial.println(F("-------- DEBUG --------"));
       statusMsg = buildMessage(CurrentGpsData,lastValidGPSData,drowsyState,
                     helmetState, maxSpeed, alcoholDetected,driverMobile, speedAlert,
                     wrongDirection,accidentState);
       Serial.println(statusMsg); 
       /* ================= TELEGRAM ================= */
       portENTER_CRITICAL(&statusMux);
       TelegramstatusMsg = statusMsg ;
       portEXIT_CRITICAL(&statusMux);
    }
    /* ================= ALERT OUTPUT ================= */
    HandleAlerts(accidentState,wrongDirection,drowsyState, alcoholDetected,
                    speedAlert,helmetState);
    /* ================= ALERT ================= */
    if (now - lastAlertCheck >= ALERT_INTERVAL)
    {
        lastAlertCheck = now;
        if (accidentState == AD_STATE_ACCIDENT && !alertSent)
        {
            sendAccidentTelegram(statusMsg);
            Serial.println(F("Telegram Alert Triggered"));
            alertSent = true;
        }

        if (accidentState == AD_STATE_NORMAL)
            alertSent = false;
    }
    float lat = CurrentGpsData.valid ? CurrentGpsData.latitude : 0.0;
    float lon = CurrentGpsData.valid ? CurrentGpsData.longitude : 0.0;
    float speed = CurrentGpsData.valid ? CurrentGpsData.speed : 0.0;
    ignition = (alcoholDetected == 0 && helmetState == 1) ? 1 : 0;
    static String gpsDateTime = "NO_TIME";
    if (CurrentGpsData.date.valid && CurrentGpsData.time.valid)
        gpsDateTime = getISTDateTimeString(&CurrentGpsData.date, &CurrentGpsData.time);
    else if (lastValidGPSData.date.valid && lastValidGPSData.time.valid)
        gpsDateTime = getISTDateTimeString(&lastValidGPSData.date, &lastValidGPSData.time);
        /* ================= BLE ================= */
    if (now - lastBLE >= BLE_INTERVAL)
    {
        lastBLE = now;
        if (ble_isConnected())
        {
            ble_process(lat,lon,speed,maxSpeed,speedAlert, alcoholDetected, drowsyState,helmetState,
                        ignition, driverMobile,gpsDateTime.c_str(),wrongDirection,accidentState);
        }
    }
}

/* ================= TELEGRAM SEND ================= */
void sendAccidentTelegram(String msg)
{
    String message = "ACCIDENT DETECTED\n\n";
    message += msg;
    telegram_sendMessage(message);
}

/* ================= MESSAGE BUILDER ================= */
String buildMessage(GPSData current, GPSData lastValid,
                    bool isDrowsy, bool helmetState,
                    float maxAllowedSpeed,
                    bool alcoholDetected, bool driverMobile,
                    int speedAlert, bool wrongDirection,
                    int accidentState)
{
    String message;
    message.reserve(1500);
    // -------- TIME --------
    String timeStr;
    if (current.date.valid && current.time.valid)
        timeStr = getISTDateTimeString(&current.date, &current.time);
    else if (lastValid.date.valid && lastValid.time.valid)
        timeStr = getISTDateTimeString(&lastValid.date, &lastValid.time);
    else
        timeStr = "Not available";
    message += "TIME (IST)     : " + timeStr + " 🕒\n";
    // -------- GOOGLE MAP (TOP PRIORITY) --------
    if (current.valid)
    {
        String mapLink = "https://maps.google.com/?q=" +
                         String(current.latitude, 6) + "," +
                         String(current.longitude, 6);
        message += "MAP            : " + mapLink + " 📍\n";
    }
    else if (lastValid.valid)
    {
        String mapLink = "https://maps.google.com/?q=" +
                         String(lastValid.latitude, 6) + "," +
                         String(lastValid.longitude, 6);
        message += "MAP            : " + mapLink + " (last) 📍\n";
    }
    else
    {
        message += "MAP            : Not available\n";
    }
    // -------- LOCATION --------
    if (current.valid)
    {
        message += "LAT            : " + String(current.latitude, 6) + " 📍\n";
        message += "LON            : " + String(current.longitude, 6) + " 📍\n";
    }
    else if (lastValid.valid)
    {
        message += "LAT            : " + String(lastValid.latitude, 6) + " 📍 (last)\n";
        message += "LON            : " + String(lastValid.longitude, 6) + " 📍 (last)\n";
    }
    else
    {
        message += "LAT            : Not available\n";
        message += "LON            : Not available\n";
    }
    // -------- SPEED --------
    message += "SPEED          : " + String(current.speed, 1) + " km/h 🧭\n";
    message += "MAX SPEED      : " + String(maxAllowedSpeed, 1) + " km/h 🛑\n";
    // -------- SPEED ALERT --------
    message += "SPEED ALERT    : ";
    switch (speedAlert)
    {
        case 1: message += "OVER SPEED 🔴\n"; break;
        case 2: message += "NEAR LIMIT 🟡\n"; break;
        case 0: message += "SAFE 🟢\n"; break;
        default: message += "UNKNOWN ❓\n"; break;
    }
    // -------- ALCOHOL --------
    message += "ALCOHOL        : ";
    message += alcoholDetected ? "DETECTED 🍺\n" : "NO ALCOHOL 🟢\n";
    // -------- DROWSY --------
    message += "DROWSY         : ";
    message += isDrowsy ? "DROWSY 😴\n" : "AWAKE 🟢\n";
    // -------- HELMET --------
    message += "HELMET WEAR    : ";
    message += helmetState ? "WORN 🟢\n" : "NOT WORN 🔴\n";
    // -------- IGNITION --------
    message += "IGNITION       : ";
    message += (ignition ? "ON 🔑\n" : "OFF ⛔\n");
    // -------- MOBILE --------
    message += "MOBILE         : ";
    message += driverMobile ? "CONNECTED 📶\n" : "NOT CONNECTED ❌\n";
    // -------- ACCIDENT --------
    message += "ACCIDENT STATE : ";
    message += (accidentState == 0) ? "NO ACCIDENT 🟢\n" :
           (accidentState == 1) ? "ACCIDENT DETECTED 🔴\n" :
                                  "UNKNOWN ❓\n";
    // -------- DIRECTION --------
    message += "DIRECTION      : ";
    message += wrongDirection ? "WRONG WAY 🔴\n" : "CORRECT ✔️\n";
    return message;
}

