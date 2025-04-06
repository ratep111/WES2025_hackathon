# 🚨 WES2025 Hackathon – Embedded Infotainment System

**Team #4 - Firma/Depwesija**
- **Majda Bakmaz**
- **Luka Bradaric-Lisic**
- **Petar Jakus**
- **Jakov Jovic**

A modular, embedded infotainment and monitoring system designed for **emergency vehicles** such as ambulances, firetrucks, and police units.  
Built with **ESP-IDF**, **FreeRTOS**, and **LVGL**, this system integrates with onboard sensors to provide real-time feedback, environmental monitoring, and user-friendly interaction through a touch display.

---

## 🚀 Features

- 🌗 **Day/Night Detection** – VEML7700 sensor for automatic display brightness adaptation
- 🚪 **IR Door Sensor** – TCRT5000 for door open/close detection
- 🏎️ **Acceleration + Speed Estimation** – Using LIS2DH12TR accelerometer
- 📏 **Ultrasonic Parking Sensor** – Distance detection with optional buzzer feedback
- 🌡️ **Temperature + Humidity Monitoring** – Via SHT3x sensor
- 🕒 **Real-Time Clock (RTC)** – PCF8523 with battery backup support
- 🎮 **Input Devices** – Joystick and tactile buttons
- 🖥️ **LVGL GUI** – Real-time info and alerts on a touch-enabled TFT display
- 🧩 **Modular Architecture** – Clean FreeRTOS components for drivers and logic

---

## 🛠 Getting Started

> Requires **ESP-IDF v5.0.2**

### 🧰 Project Initialization

Clone the repo and run:

```bash
./project_init.sh
```

This will:

- Initialize Git submodules
- Set up pre-commit hooks
- Patch ESP-IDF drivers (if needed)
- Check and install `clang-format`
- Load recommended `sdkconfig` for the BLDK devkit

---

### 🔨 Build + Flash

To compile the project:

```bash
idf.py build
```

To flash and monitor:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

---

## 📁 Project Structure

| Directory     | Description                                            |
| ------------- | ------------------------------------------------------ |
| `main/`       | Main application entry point, task management          |
| `components/` | Hardware drivers and modular FreeRTOS-compatible logic |
| `templates/`  | Base templates for reusable components                 |
| `doc/`        | Hardware documentation and schematics                  |
| `scripts/`    | Tooling (e.g., formatter hook, CI helpers)             |
| `config/`     | Kconfig files and ESP-IDF project configs              |

---

## 🧩 Modular Component System

All sensors and logic are implemented as **modular components** in `components/`.

For a full list of available modules, their GPIOs, and capabilities, check:  
📄 [`components/README.md`](components/README.md)

---

## 🔌 Hardware Platform

- **ESP32 WROVER module**
- Custom baseboard with:
  - TCRT5000 IR reflective sensors (door)
  - VEML7700 ambient light sensor
  - LIS2DH12TR SPI accelerometer
  - HC-SR04 ultrasonic distance sensor
  - PCF8523 RTC with CR1220 backup
  - SHT3x-DIS temperature & humidity sensor
  - Buzzer + 3x LED indicators
  - Push buttons and joystick
  - 2.4" or 3.5" TFT display (LVGL-ready)

---

## 📓 Development Notes

- Written using **ESP-IDF 5.x** with proper component registration
- Hardware drivers separated cleanly from app logic
- Real-time safe and fully non-blocking sensor tasks
- Components are lightweight, reusable, and testable

---
