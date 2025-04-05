# 📦 Components Directory

This directory contains modular components used in the WES2025 Hackathon project. It includes both:

- **Driver components**: for interfacing with hardware peripherals (sensors, IO, etc.)
- **Application-level components**: which process sensor data and provide higher-level functionality

---

## 🧩 App Components

These components contain logic that **processes sensor data**, handles **event detection**, or implements **interactive features** for the user interface.

| Component                | Description                                                                                                                |
| ------------------------ | -------------------------------------------------------------------------------------------------------------------------- |
| `app-day-night-detector` | Detects ambient light level using VEML7700 to determine day/night state.                                                   |
| `app-door-detector`      | Uses a TCRT5000 infrared sensor to detect if a door is open or closed.                                                     |
| `app-parking-sensor`     | Measures distance using HC-SR04 and provides audio proximity feedback via buzzer. _(LEDs deprecated)_                      |
| `app-speed-estimator`    | Computes speed and movement direction from LIS2DH12TR accelerometer data. Provides real-time velocity in multiple formats. |

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
| `button`             | GPIO-based button handler with callbacks                  |

---

## 🖥 GUI Integration

- The `gui` component is built using LVGL (LittlevGL), and is responsible for rendering interface elements.
- It retrieves state (e.g. speed, light, proximity) from app components via clean APIs.

---

## 📌 GPIO Pin Mapping

| Peripheral       | Pin Macro                | GPIO | Notes                                  |
| ---------------- | ------------------------ | ---- | -------------------------------------- |
| HC-SR04 Trigger  | `ULTRASONIC_TRIGGER_PIN` | 27   | Shared with `LED_G`                    |
| HC-SR04 Echo     | `ULTRASONIC_ECHO_PIN`    | 34   | Shared with `JOY_X`                    |
| TCRT5000 Digital | `TCRT5000_DIGITAL_PIN`   | 35   | Digital input for door state detection |

> ⚠️ Make sure not to reuse these pins in other components unless explicitly multiplexed.

---

## 🧠 App Component APIs

### Speed Estimator API

| Function                                 | Description                                              |
| ---------------------------------------- | -------------------------------------------------------- |
| `speed_estimator_init()`                 | Initialize the accelerometer and speed estimation system |
| `speed_estimator_get_speed_mps()`        | Get current speed in meters per second                   |
| `speed_estimator_get_speed_kmh()`        | Get current speed in kilometers per hour                 |
| `speed_estimator_get_direction()`        | Get enum value of current movement direction             |
| `speed_estimator_get_direction_string()` | Get human-readable direction (Forward, Backward, etc.)   |
| `speed_estimator_is_moving_forward()`    | Boolean check if movement is forward                     |
| `speed_estimator_is_moving_backward()`   | Boolean check if movement is backward                    |

### Parking Sensor API

| Function                        | Description                                                    |
| ------------------------------- | -------------------------------------------------------------- |
| `parking_sensor_init()`         | Initialize the ultrasonic sensor and distance detection system |
| `parking_sensor_get_distance()` | Retrieve current distance measurement in centimeters           |
| `parking_sensor_is_danger()`    | Check if object is in danger zone (<30cm)                      |
| `parking_sensor_is_warning()`   | Check if object is in warning zone (30-80cm)                   |
| `parking_sensor_is_safe()`      | Check if object is in safe zone (>80cm)                        |

### Door Detector API

| Function                   | Description                                                 |
| -------------------------- | ----------------------------------------------------------- |
| `door_detector_init()`     | Initialize the IR sensor for door state detection           |
| `is_door_open()`           | Check if door is currently in open state                    |
| `is_door_closed()`         | Check if door is currently in closed state                  |
| `get_door_state()`         | Get current door state enum value                           |
| `get_door_event()`         | Retrieve door state change events from queue with timestamp |
| `door_register_callback()` | Register function to be called on door state changes        |

### Day/Night Detector API

| Function                    | Description                                             |
| --------------------------- | ------------------------------------------------------- |
| `day_night_init()`          | Initialize the ambient light sensor and state detection |
| `is_day_mode()`             | Check if current light level indicates day time         |
| `is_night_mode()`           | Check if current light level indicates night time       |
| `get_light_level()`         | Retrieve current light level in lux                     |
| `get_light_state()`         | Get current light state enum value                      |
| `light_register_callback()` | Register function to be called on light state changes   |

---

## 🛠 Utility/Support Components

| Component             | Role                                          |
| --------------------- | --------------------------------------------- |
| `esp32-perfmon`       | Performance monitoring wrappers               |
| `esp_idf_lib_helpers` | Generic ESP-IDF C helpers (I2C, delays, etc.) |
| `json`                | Likely a wrapper for parsing/generating JSON  |

---
