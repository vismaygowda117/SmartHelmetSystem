# IoT Based Smart Helmet System

An IoT-enabled Smart Helmet System developed to improve two-wheeler rider safety using ESP32 microcontrollers, multiple sensors, GPS, Bluetooth Low Energy (BLE), and Telegram notifications.

The system continuously monitors the rider's condition and the vehicle's status, providing real-time alerts for unsafe situations such as riding without a helmet, alcohol consumption, drowsiness, over-speeding, wrong-direction travel, and accidents.

---

## Features

- Helmet detection before vehicle ignition
- Alcohol detection using MQ-3 sensor
- Drowsiness detection using an eye blink sensor
- Accident detection using MPU6050
- GPS-based location and speed monitoring
- Road speed limit monitoring
- Wrong-direction detection
- Bluetooth Low Energy (BLE) communication between helmet and bike units
- Smart ignition control
- Real-time LCD status display
- Telegram emergency notifications with GPS location
- Audio, visual, and vibration alerts

---

## System Architecture

The project consists of two separate embedded units.

### Helmet Unit (ESP32-S3)

- Helmet detection sensor
- MQ-3 Alcohol Sensor
- Eye Blink Sensor
- MPU6050
- GPS Neo-7M
- Buzzer
- Vibration Motor
- Dual Colour LED
- BLE Communication
- Wi-Fi for Telegram notifications

### Bike Unit (ESP32-C3 Super Mini)

- BLE Communication
- Relay Module for ignition control
- 20×4 LCD Display
- I2C Level Shifter
- RGB Status LEDs
- Bike Key/Ignition Simulation

---

## Working Principle

1. The rider wears the helmet.
2. The helmet unit checks whether the helmet is properly worn.
3. The MQ-3 sensor checks for alcohol.
4. The eye blink sensor monitors rider drowsiness.
5. The MPU6050 monitors sudden impacts for accident detection.
6. GPS continuously tracks vehicle speed and location.
7. Helmet data is transmitted to the bike unit via BLE.
8. The bike unit enables ignition only when safety conditions are satisfied.
9. If an accident is detected, the system sends a Telegram message containing the rider's GPS location.

---

## Hardware Used

### Helmet Unit

- ESP32-S3
- MQ-3 Alcohol Sensor
- IR Helmet Detection Sensor
- Eye Blink Sensor
- MPU6050
- GPS Neo-7M
- Buzzer
- Coin Vibration Motor
- Dual Colour LED
- Li-ion Battery

### Bike Unit

- ESP32-C3 Super Mini
- Relay Module
- 20×4 LCD Display
- I2C Bidirectional Level Shifter
- RGB LEDs
- Bike Key Switch
- DC Motor (Ignition Simulation)

---

## Software Used

- Arduino IDE
- Embedded C++
- FreeRTOS
- Bluetooth Low Energy (BLE)
- Wi-Fi
- Telegram Bot API

---

## Future Improvements

- AI-based accident prediction
- Voice-based rider alerts
- Mobile application
- Health monitoring sensors
- Cloud dashboard
- Vehicle-to-Vehicle communication
- Camera-based pothole detection
- Improved battery management

---

## Authors

- Vismay Y Gowda
- Vishnuvardhan D
- Sriram Adityeswar Darla
- Srividya MS

Guide: Dr. Raveesha

---

## License

This project was developed as an academic mini project at CMR Institute of Technology and is intended for educational purposes.
