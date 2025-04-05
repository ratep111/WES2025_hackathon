# WES2025 Hackathon – Embedded Infotainment System

This project implements a modular, embedded infotainment and monitoring system for emergency vehicles including ambulances, firetrucks, and police vehicles. The system is designed to interface with various onboard sensors, provide critical feedback to personnel, and display contextual data via a touch-based GUI using LVGL.

---

## Features

- Ambient light detection (day/night) for dynamic display adaptation
- Door open/close detection using IR reflective sensors
- Real-time acceleration and speed estimation via LIS2DH12TR
- Parking/distance sensor with audible buzzer feedback
- Temperature and humidity sensing (SHT3x)
- Real-time clock (PCF8523) with backup battery support
- Joystick and button input support
- LVGL-powered GUI for information and alerts
- Modular FreeRTOS-based component architecture

---

## Getting Started

Make sure you have ESP-IDF v5.0.2 installed before proceeding.

### Project Initialization

Clone the repo and run:

    ./project_init.sh

This script will:

- Initialize Git submodules
- Set up pre-commit hooks
- Patch relevant ESP-IDF drivers if needed
- Check/install clang-format
- Prepare sdkconfig for the BLDK devkit

### Building

    idf.py build

To flash and monitor:

    idf.py -p /dev/ttyUSB0 flash monitor

---

## Project Structure

| Directory   | Description                                                |
| ----------- | ---------------------------------------------------------- |
| main/       | Main app logic (entry point, task spawning)                |
| components/ | Drivers and FreeRTOS-compatible app components             |
| templates/  | Starter templates for new components or apps               |
| doc/        | Hardware documentation and schematics                      |
| scripts/    | Dev tools and format hooks (e.g., pre-commit-formatter.sh) |
| config/     | Project and Kconfig definitions                            |

---

## Modular Components

All drivers and logical app features are implemented as standalone components in `components/`.

For a full list of components, their purpose, and GPIO/pin mappings, see:
components/README.md

---

## Hardware Platform

- ESP32 WROVER module
- Baseboard with:
  - TCRT5000 IR sensors
  - VEML7700 ambient light sensor
  - LIS2DH12TR accelerometer (SPI)
  - HC-SR04 ultrasonic sensor
  - PCF8523T RTC
  - SHT3x-DIS temp/humidity sensor
  - 3x LED + buzzer output
  - Buttons & joystick input
  - TFT display (LVGL-ready)

---

## Development Notes

- Built on FreeRTOS using ESP-IDF 5.x style component registration
- Clean separation between hardware drivers and application logic
- Designed for clarity, modularity, and real-time responsiveness
- All sensor tasks are independent and non-blocking

---

## Contributing

- Run ./project_init.sh after cloning to set up your environment
- Use clang-format and respect existing code style
- All features should be implemented as reusable components when possible

---

## License

This repository is part of the WES2025 Hackathon initiative. Licensing is under internal use only unless otherwise stated.

---
