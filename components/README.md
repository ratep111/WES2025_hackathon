# 📦 Components Directory

This directory contains modular components used in the WES2025 Hackathon project. It includes both:

- **Driver components**: for interfacing with hardware peripherals (sensors, IO, etc.)
- **Application-level components**: which process sensor data and provide higher-level functionality

---

## 🧩 App Components

These components contain logic that **processes sensor data**, handles **event detection**, or implements **interactive features** for the user interface.

| Component                | Description                                                                                                                |
| ------------------------ | -------------------------------------------------------------------------------------------------------------------------- |
| `app-acc-data-provider`  | Centralized accelerometer data provider that reduces SPI bus contention and provides filtered sensor data to consumers.    |
| `app-crash-detector`     | Detects impact events using the accelerometer data and triggers notifications via I/O expander.                            |
| `app-day-night-detector` | Detects ambient light level using VEML7700 to determine day/night state.                                                   |
| `app-door-detector`      | Uses a TCRT5000 infrared sensor via I/O expander to detect if a door is open or closed.                                    |
| `app-mqtt`               | Handles MQTT communication for sending sensor data to cloud services.                                                      |
| `app-parking-sensor`     | Measures distance using HC-SR04 and provides audio proximity feedback via connected speaker circuit.                       |
| `app-speed-estimator`    | Computes speed and movement direction from LIS2DH12TR accelerometer data. Provides real-time velocity in multiple formats. |
| `gui_controller`         | Connects sensor modules to the GUI frontend, handling data flow and event management between components.                   |

These components often expose **public APIs** to be consumed by the `main` app or the `gui`.

---

## 🔌 Peripheral Drivers

These components provide low-level access to sensors and hardware interfaces. They wrap I2C/SPI/UART communication and expose C functions for use in app components.

| Component             | Purpose                                                   |
| --------------------- | --------------------------------------------------------- |
| `acc-LIS2DH12TR`      | SPI driver for the LIS2DH12TR 3-axis accelerometer        |
| `als-veml7700`        | I2C driver for the VEML7700 ambient light sensor          |
| `infrared-tcrt5000`   | GPIO/ADC driver for TCRT5000 IR reflective sensor         |
| `ultrasonic-hc-sr04`  | GPIO driver for HC-SR04 ultrasonic distance sensor        |
| `rtc-pcf8523t`        | I2C driver for PCF8523T real-time clock                   |
| `sht3x-dis`           | I2C driver for SHT3x-DIS temperature and humidity sensors |
| `joystick`            | Reads analog position of joystick via ADC                 |
| `led`, `buzzer`       | Simple GPIO/PWM drivers for output devices                |
| `button`              | GPIO-based button handler with callbacks                  |
| `io-expander-pcf8574` | I2C driver for PCF8574 I/O expander used to extend GPIO   |
| `speaker`             | Audio output via I2S DAC interface                        |
| `i2cdev`              | Generic I2C device communication helper                   |
| `eeprom`              | I2C driver for AT24CX EEPROM storage                      |

## 🖥 GUI Integration

- The `gui` component is built using LVGL (LittlevGL), and is responsible for rendering interface elements.
- It retrieves state (e.g. speed, light, proximity) from app components via clean APIs.

---

## 📌 Hardware Connection Documentation

This part provides detailed information about hardware connections for the project components.

## Ultrasonic Sensor (HC-SR04)

The ultrasonic sensor is used for distance measurement in the parking sensor application.

| Sensor Pin | Connected To | GPIO    | Notes                                    |
| ---------- | ------------ | ------- | ---------------------------------------- |
| GND        | GND          | -       | Ground connection                        |
| ECHO       | JOY_X        | GPIO_34 | Echo pin (receives the reflected signal) |
| TRIG       | LED_G        | GPIO_27 | Trigger pin (sends the ultrasonic pulse) |
| VCC        | 5V           | -       | Power supply (5V required)               |

## Ambient Light Sensor (VEML7700)

The ambient light sensor is used for day/night detection.

| Sensor Pin | Connected To | GPIO    | Notes               |
| ---------- | ------------ | ------- | ------------------- |
| SCL        | SCL          | GPIO_21 | I2C clock line      |
| SDA        | SDA          | GPIO_22 | I2C data line       |
| GND        | GND          | -       | Ground connection   |
| VIN        | 3V3          | -       | Power supply (3.3V) |

## Infrared Sensor (TCRT5000)

The infrared sensor is used for door state detection.

| Sensor Pin | Connected To   | GPIO | Notes                           |
| ---------- | -------------- | ---- | ------------------------------- |
| VCC        | 5V             | -    | Power supply (5V)               |
| GND        | GND            | -    | Ground connection               |
| DIGITAL    | Expander Pin 1 | -    | Digital output via I/O expander |

## ESP32-CAM Module

| Module Pin | Connected To   | GPIO    | Notes                           |
| ---------- | -------------- | ------- | ------------------------------- |
| GPIO_12    | Expander Pin 0 | GPIO_12 | Camera control via I/O expander |
| VIN        | 5V             | -       | Power supply (5V)               |
| GND        | GND            | -       | Ground connection               |

## Analog Speaker Circuit

| Circuit Pin | Connected To | GPIO    | Notes                                               |
| ----------- | ------------ | ------- | --------------------------------------------------- |
| VCC         | 5V           | -       | Power supply (5V)                                   |
| GND         | GND          | -       | Ground connection                                   |
| VIN         | BTN_4        | GPIO_27 | Audio input signal (shared with ultrasonic trigger) |

## I/O Expander (PCF8574)

| Expander Pin | Connected To      | Function                  |
| ------------ | ----------------- | ------------------------- |
| P0           | ESP32-CAM GPIO_12 | Camera control            |
| P1           | TCRT5000 DIGITAL  | Door sensor digital input |
| VCC          | 3V3               | Power supply (3.3V)       |
| GND          | GND               | Ground connection         |
| SCL          | SCL               | I2C clock line (GPIO_21)  |
| SDA          | SDA               | I2C data line (GPIO_22)   |

## Accelerometer (LIS2DH12TR)

| Sensor Pin | Connected To | GPIO    | Notes                    |
| ---------- | ------------ | ------- | ------------------------ |
| SCK        | VSPI_CLK     | GPIO_18 | SPI clock                |
| MISO       | VSPI_MISO    | GPIO_19 | SPI master in, slave out |
| MOSI       | VSPI_MOSI    | GPIO_23 | SPI master out, slave in |
| CS         | VSPI_CS      | GPIO_5  | SPI chip select          |
| VDD        | 3V3          | -       | Power supply (3.3V)      |
| GND        | GND          | -       | Ground connection        |

## Temperature/Humidity Sensor (SHT3x)

| Sensor Pin | Connected To | GPIO    | Notes               |
| ---------- | ------------ | ------- | ------------------- |
| SCL        | SCL          | GPIO_21 | I2C clock line      |
| SDA        | SDA          | GPIO_22 | I2C data line       |
| VDD        | 3V3          | -       | Power supply (3.3V) |
| GND        | GND          | -       | Ground connection   |

## RTC Module (PCF8523T)

| Module Pin | Connected To | GPIO    | Notes               |
| ---------- | ------------ | ------- | ------------------- |
| SCL        | SCL          | GPIO_21 | I2C clock line      |
| SDA        | SDA          | GPIO_22 | I2C data line       |
| VCC        | 3V3          | -       | Power supply (3.3V) |
| GND        | GND          | -       | Ground connection   |

## Joystick

| Joystick Pin | Connected To | GPIO    | Notes                       |
| ------------ | ------------ | ------- | --------------------------- |
| X            | ADC          | GPIO_34 | Shared with ultrasonic ECHO |
| Y            | ADC          | GPIO_35 | Analog Y-axis input         |
| BTN          | GPIO         | GPIO_36 | Button press detection      |
| VCC          | 3V3          | -       | Power supply (3.3V)         |
| GND          | GND          | -       | Ground connection           |

## Button Connections

| Button | GPIO    | Notes                                         |
| ------ | ------- | --------------------------------------------- |
| BTN_1  | GPIO_39 | General purpose button                        |
| BTN_2  | GPIO_38 | General purpose button                        |
| BTN_3  | GPIO_37 | General purpose button                        |
| BTN_4  | GPIO_27 | Shared with ultrasonic TRIG and speaker input |

## LED Connections

| LED   | GPIO    | Notes     |
| ----- | ------- | --------- |
| LED_R | GPIO_25 | Red LED   |
| LED_G | GPIO_26 | Green LED |
| LED_B | GPIO_32 | Blue LED  |

## Important Notes

1. **Shared Pins**: Several GPIO pins are shared between multiple components:

   - GPIO_27 (BTN_4) is shared with the ultrasonic sensor trigger and speaker input
   - GPIO_34 (JOY_X) is shared with the ultrasonic sensor echo

2. **I2C Bus**: Multiple components share the same I2C bus:

   - SDA: GPIO_22
   - SCL: GPIO_21
   - Devices: VEML7700 (light sensor), PCF8574 (I/O expander), SHT3x (temp/humidity sensor), PCF8523T (RTC)

3. **SPI Bus**: The LIS2DH12TR accelerometer uses the VSPI interface:

   - SCK: -
   - MISO: -
   - MOSI: -
   - CS: -

4. **I/O Expander**: The PCF8574 I/O expander is used to expand available GPIO pins:
   - Pin 0: Connected to ESP32-CAM GPIO_12
   - Pin 1: Connected to TCRT5000 infrared sensor digital output
   - Pin 2-8: Unused

This documentation should be kept updated as hardware connections change during development.

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
