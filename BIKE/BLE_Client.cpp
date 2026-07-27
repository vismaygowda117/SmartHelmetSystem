#include "ble_client.h"
#include <Arduino.h>
#include <stdio.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLERemoteCharacteristic.h>

// UUIDs
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef123456"

// Globals
static BLEAddress serverAddress("00:00:00:00:00:00");
static BLEClient* client = nullptr;
static BLERemoteCharacteristic* remoteChar = nullptr;

static bool deviceFound = false;
bool BLEconnected = false;
bool helmetDataValid = false;

// STATES
int driverMobileState = false;

// Buffer
volatile bool newData = false;
char bleBuffer[220];

double lat = 0;
double lon = 0;
double speed = 0;
double maxSpeed = 0;

int speedAlert = 0;
int alcoholAlert = 0;
int drowsyAlert = 0;
int helmetWearAlert = 0;
int ignitionStatus = 0;
int directionAlert = 0;
int AccidentState = 0;

char gpsTime[50] = {0};

// ---------------- NOTIFY CALLBACK ----------------
void notifyCallback(
    BLERemoteCharacteristic* pChar,
    uint8_t* data,
    size_t length,
    bool isNotify)
{
    size_t len = length;
    if (len >= sizeof(bleBuffer))
        len = sizeof(bleBuffer) - 1;

    memcpy(bleBuffer, data, len);
    bleBuffer[len] = '\0';

    newData = true;
}

// ---------------- PROCESS BLE ----------------
void processBLE()
{
    if (!newData) return;

    char localBuffer[260];
    memcpy(localBuffer, bleBuffer, sizeof(localBuffer));
    localBuffer[sizeof(localBuffer) - 1] = '\0';

    newData = false;

    int parsed = sscanf(localBuffer,
        "LAT:%lf,LON:%lf,SPD:%lf,MAX:%lf,ALR:%d,ALC:%d,DRW:%d,HLM:%d,IGN:%d,MOB:%d,T:%49[^,],DIR:%d,ACD:%d",
        &lat,&lon,&speed,&maxSpeed,&speedAlert,&alcoholAlert,&drowsyAlert,&helmetWearAlert,&ignitionStatus,
        &driverMobileState,gpsTime,&directionAlert,&AccidentState);
    if (parsed == 13)    
    {
        helmetDataValid = true;
    }
    else
    {
       // Serial.println("❌ PARSE FAILED");
        helmetDataValid = false;
        resetHelmetData();
    }
}
// ---------------- SCAN CALLBACK ----------------
class ScanCB : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice device)
    {
        if (device.getName() == "ESP32_Vehicle")
        {
            Serial.println("Helmet found!");
            serverAddress = BLEAddress(device.getAddress());
            deviceFound = true;
            BLEDevice::getScan()->stop();
        }
    }
};

// ---------------- CLIENT CALLBACK ----------------
class ClientCB : public BLEClientCallbacks
{
    void onConnect(BLEClient* pClient)
    {
        Serial.println("Connected to helmet");
        BLEconnected = true;
    }

    void onDisconnect(BLEClient* pClient)
    {
        Serial.println("Disconnected from helmet");

        BLEconnected = false;
        deviceFound = false;
        newData = false;
        helmetDataValid = false;
        resetHelmetData();

        remoteChar = nullptr;

        if (client)
        {
            client->disconnect();
            delete client;
            client = nullptr;
        }

        delay(100);

        BLEDevice::getScan()->start(0, nullptr);
    }
};

// ---------------- CONNECT ----------------
bool connectToServer()
{
    if (!client)
    {
        client = BLEDevice::createClient();
        client->setClientCallbacks(new ClientCB());
        client->setMTU(200);
    }
    if (!client->connect(serverAddress))
    {
        Serial.println("Connect failed");
        return false;
    }
    BLERemoteService* service = client->getService(SERVICE_UUID);
    if (!service) return false;
    remoteChar = service->getCharacteristic(CHARACTERISTIC_UUID);
    if (!remoteChar) return false;
    if (remoteChar->canNotify())
        remoteChar->registerForNotify(notifyCallback);
    BLEconnected = true;
    return true;
}

// ---------------- SCAN ----------------
void startScan()
{
    BLEScan* scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new ScanCB());
    scan->setActiveScan(true);
    scan->start(2, false);
}

// ---------------- INIT ----------------
void ble_client_init()
{
    resetHelmetData();
    BLEDevice::init("");
    BLEDevice::setMTU(200);
    startScan();
}

// ---------------- LOOP ----------------
void ble_client_loop()
{
    processBLE();

    static bool lastConnected = false;
    static bool lastDeviceFound = false;
    if (BLEconnected != lastConnected)
    {
        lastConnected = BLEconnected;
        Serial.println(BLEconnected ? "BLE CONNECTED" : "BLE DISCONNECTED");
    }
    if (deviceFound != lastDeviceFound)
    {
        lastDeviceFound = deviceFound;
        Serial.println(deviceFound ? "HELMET FOUND" : "HELMET LOST");
    }
    if (deviceFound && !BLEconnected)
    {
        if (!connectToServer())
            deviceFound = false;
    }
    if (!BLEconnected)
    {
        BLEScan* scan = BLEDevice::getScan();
        if (!scan->isScanning())
            startScan();
    }
}

void resetHelmetData()
{
    lat = 0;
    lon = 0;
    speed = 0;
    maxSpeed = 0;
    speedAlert = 0;
    alcoholAlert = 0;
    drowsyAlert = 0;
    helmetWearAlert = 0;
    ignitionStatus = 0;
    directionAlert = 0;
    AccidentState = 0;
    memset(gpsTime, 0, sizeof(gpsTime));
    strcpy(gpsTime, "NO GPS TIME");    
    driverMobileState = false;
}

void PrintHelmetDebug()
{
    Serial.println("---------BIKE UNIT - PARSED VALUES --------");
    Serial.printf("TIME           : %s 🕒\n", gpsTime);
    Serial.printf("LAT            : %.6f 📍\n", lat);
    Serial.printf("LON            : %.6f 📍\n", lon);
    Serial.printf("SPEED          : %.2f km/h 🧭\n", speed);
    Serial.printf("MAX SPEED      : %.2f km/h 🛑\n", maxSpeed);
    Serial.printf("SPEED ALERT    : %s\n",
        (speedAlert == 1) ? "OVER SPEED 🔴" :
        (speedAlert == 2) ? "NEAR LIMIT 🟡" :
                            "SAFE 🟢");
    Serial.printf("ALCOHOL        : %s\n",
        alcoholAlert ? "DETECTED 🍺" : "NO ALCOHOL 🟢");
    Serial.printf("DROWSY         : %s\n",
        drowsyAlert ? "DROWSY 😴" : "AWAKE 🟢");
    Serial.printf("HELMET WEAR    : %s\n",
        helmetWearAlert ? "WORN 🟢" : "NOT WORN 🔴");
    Serial.printf("IGNITION       : %s\n",
        ignitionStatus ? "ON 🔑" : "OFF ⛔");
    Serial.printf("MOBILE         : %s\n",
        driverMobileState ? "CONNECTED 📶" : "NOT CONNECTED ❌");
    const char* accidentStr =
        (AccidentState == 0) ? "NO ACCIDENT 🟢" :
        (AccidentState == 1) ? "ACCIDENT DETECTED 🔴" :
                              "UNKNOWN ❓";
    Serial.printf("ACCIDENT STATE : %s\n", accidentStr);
    const char* dirStr =
        (directionAlert == 0) ? "CORRECT ✔️" :
        (directionAlert == 1) ? "WRONG WAY 🔴" :
                                "NOT MOVING ⏸️";
    Serial.printf("DIRECTION      : %s\n", dirStr);
}


bool IsHelmetDataValid()
{
    return helmetDataValid;
}