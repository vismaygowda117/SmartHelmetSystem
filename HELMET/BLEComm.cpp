#include "BLEComm.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ---------------- UUIDs ----------------
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef123456"

// ---------------- GLOBALS ----------------
static BLECharacteristic *pCharacteristic = nullptr;
static bool deviceConnected = false;

// ---------------- CALLBACK ----------------
class ServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer* pServer)
    {
        deviceConnected = true;
        Serial.println("Client Connected");
    }
    void onDisconnect(BLEServer* pServer)
    {
        deviceConnected = false;
        Serial.println("Client Disconnected");
        // CRITICAL: restart advertising immediately
        BLEDevice::startAdvertising();
    }
};

// ---------------- INIT ----------------
void BLE_init()
{
    BLEDevice::init("ESP32_Vehicle");
    BLEDevice::setMTU(200);
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();
    // Improve connection speed
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Helmet BLE Server Started");
}

// ---------------- STATUS ----------------
bool ble_isConnected()
{
    return deviceConnected;
}

void ble_process(double lat, double lon, double speed, float maxSpeed,
                 int speedAlert, int alcohol, int drowsy,
                 int helmetWear,int ignition, int driverMobile,
                 const char* gpsDateTime,int direction, int accidentState)
{
    if (!deviceConnected || pCharacteristic == nullptr)
        return;
    char data[260];
    snprintf(data, sizeof(data),
        "LAT:%.6f,LON:%.6f,SPD:%.2f,MAX:%.2f,ALR:%d,ALC:%d,DRW:%d,HLM:%d,IGN:%d,MOB:%d,T:%s,DIR:%d,ACD:%d",
        lat, lon, speed, maxSpeed, speedAlert, alcohol, drowsy, helmetWear, ignition,
        driverMobile, gpsDateTime, direction, accidentState);
    pCharacteristic->setValue((uint8_t*)data, strlen(data));
    pCharacteristic->notify();
}