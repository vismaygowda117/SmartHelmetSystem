#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <Arduino.h>

void telegram_init();
void telegram_sendTo(const String& chat_id, const String& msg);
void telegram_loop(String statusMsg);
void telegram_sendMessage(const String& message);
bool telegram_isWiFiConnected();

#endif