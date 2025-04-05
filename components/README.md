# 📦 Components Directory

This directory contains modular components used in the WES2025 Hackathon project. It includes both:

- **Driver components**: for interfacing with hardware peripherals (sensors, IO, etc.)
- **Application-level components**: which process sensor data and provide higher-level functionality

---

## 🧩 App Components

These components contain logic that **processes sensor data**, handles **event detection**, or implements **interactive features** for the user interface.

| Component                         | Description                                                                                           |
| --------------------------------- | ----------------------------------------------------------------------------------------------------- |
| `app-day-night-detector`          | Detects ambient light level using VEML7700 to determine day/night state.                              |
| `app-door-detector`               | Uses a TCRT5000 infrared sensor to detect if a door is open or closed.                                |
| `app-parking-sensor`              | Measures distance using HC-SR04 and provides audio proximity feedback via buzzer. _(LEDs deprecated)_ |
| _(planned)_ `app-speed-estimator` | Computes speed from LIS2DH12TR accelerometer and exposes it to GUI.                                   |

These components often expose **public APIs** to be consumed by the `main` app or the `gui`.

---

## 🔌 Peripheral Drivers

These components provide low-level access to sensors and hardware interfaces. They wrap I2C/SPI/UART communication and expose C functions for use in app components.

| Component            | Purpose                                                   |
| -------------------- | --------------------------------------------------------- |
| `acc-LIS2DH12TR`     | SPI driver for the LIS2DH12TR 3-axis accelerometer        |
| `als-veml7700`       | I2C driver for the VEML7700 ambient light sensor          |
| `infrared-tcrt5000`  | GPIO/ADC driver for TCRT5000 IR reflective sensor         |
| `ultrasonic-hc-sr04` | GPIO driver for HC-SR04 ultrasonic distance sensor        |
| `ultrasonic_i2c`     | I2C driver variant for ultrasonic sensors                 |
| `rtc-pcf8523t`       | I2C driver for PCF8523T real-time clock                   |
| `sht3x-dis`          | I2C driver for SHT3x-DIS temperature and humidity sensors |
| `joystick`           | Reads analog position of joystick via ADC                 |
| `led`, `buzzer`      | Simple GPIO/PWM drivers for output devices                |

---

## 🖥 GUI Integration

- The `gui` component is built using LVGL (LittlevGL), and is responsible for rendering interface elements.
- It retrieves state (e.g. speed, light, proximity) from app components via clean APIs.

---

## 🛠 Utility/Support Components

| Component             | Role                                          |
| --------------------- | --------------------------------------------- |
| `esp32-perfmon`       | Performance monitoring wrappers               |
| `esp_idf_lib_helpers` | Generic ESP-IDF C helpers (I2C, delays, etc.) |
| `json`                | Likely a wrapper for parsing/generating JSON  |
| `button`              | GPIO-based button handler with callbacks      |

---

## 🚧 Notes

- All components follow standard ESP-IDF build system via `idf_component_register(...)` in their respective `CMakeLists.txt` files.
- Components can be added to other components via `REQUIRES` or `PRIV_REQUIRES` to manage dependencies cleanly.

---

## 🗂 Directory Layout Example
