#include "WiFiComm.h"
#include <WiFi.h>
#include "Secrets.h"

static unsigned long lastReconnect = 0;

// Initialize WiFi connection using stored credentials
void WiFiComm_init()
{
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    unsigned long start = millis();
    const unsigned long WIFI_TIMEOUT = 10000;
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT)
    {
        delay(300);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED)
        Serial.println("\nWiFi Connected");
    else
        Serial.println("\nWiFi Failed (continuing without blocking)");
}

// Handle WiFi reconnection logic (non-blocking)
void WiFiComm_loop()
{
    wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED)  return;
    if (status == WL_DISCONNECTED || status == WL_CONNECTION_LOST) {
        Serial.println("🔁 Reconnecting WiFi...");
        WiFi.disconnect(true);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
}

// Check if WiFi is currently connected
bool WiFiComm_isConnected()
{
    return (WiFi.status() == WL_CONNECTED);
}