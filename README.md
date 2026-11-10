# Make Music with Embedded Systems: ESP32 & SuperCollider Workshop

This repository contains the hardware and software setup for a workshop exploring physical computing, microcontrollers, and live sound synthesis using an **ESP32-WROOM-32**, sensors, and **SuperCollider**.


## Project Structure
- `esp32/`: Arduino sketch running dual-core (Core 0: sensors/serial, Core 1: audio rendering).
- `supercollider/`: SuperCollider script to parse USB serial data and control the synthesizer.

## License
This project is licensed under the MIT License.
