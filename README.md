# Make Music with Embedded Systems: ESP32 & SuperCollider Workshop

This repository contains the hardware and software setup for a workshop exploring physical computing, microcontrollers, and live sound synthesis using an **ESP32-WROOM-32**, sensors, and **SuperCollider**.

## 🛠 Hardware Pinout

| Component | Pin Name | ESP32 Pin | Function Description |
| :--- | :--- | :--- | :--- |
| **HC-SR04** (Ultrasonic) | TRIG | **GPIO 25** | Digital Output (Trigger pulse) |
| | ECHO | **GPIO 26** | Digital Input (Echo return) |
| **GL5539 LDR** (Photoresistor) | Analog Out | **GPIO 34** | Analog Input (Requires a 10kΩ pull-down resistor to GND) |
| **PCM5102A** (I2S DAC) | BCK | **GPIO 27** | I2S Bit Clock |
| | DIN | **GPIO 22** | I2S Data Input |
| | LCK / LRCK | **GPIO 23** | I2S Left/Right Word Clock |
| | SCK | **GND** | System Clock (Must be connected to Ground) |
| | VIN / VCC | **5V / VIN** | Power supply for the DAC |

## 📂 Project Structure
- `esp32/`: Arduino sketch running dual-core (Core 0: sensors/serial, Core 1: audio rendering).
- `supercollider/`: SuperCollider script to parse USB serial data and control the synthesizer.

## 📄 License
This project is licensed under the MIT License.
