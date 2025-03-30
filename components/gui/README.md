# UI application

This project, created for ESP32, provides <b>three demo applications</b> for you to try out and run.

<u>Provided UI applications</u>:

- <b>squareline</b> - example created and generated from the SquareLine Studio application (graphical application for creating GUI applications)
- <b>bl_gui</b> - basic example created only by coding
- <b>lvgl_demo</b> - complex application which was provided already as a demo in LVGL repository

## Requirements

- Version of ESP-IDF required: 5.0.x
- Version of LVGL used: 8.3.x
- Byte Lab Development Kit

## How to choose an application

The application is called from within `ui_app.c` source file based on #define.
Selection of which UI application to run is configured in `Kconfig` file which can be accessed by running `idf.py menuconfig` (select <u>Component config</u>-><u>UI application</u> and choose which application to run).
It can also be changed by modifying the `sdkconfig` file and changing the appropriate #define.

## More about applications

### SquareLine

SquareLine example is a showcase application used to show that it is possible to create GUI applications easily using the SquareLine Studio application. All of the designing can be done within the SquareLine Studio.

### BL GUI

Simple application used to show how you can create sliders, buttons and screen using no GUI applications like SquareLine studio. Instead, all of it is created only by writing C code.

### LVGL demo

Complex and advanced application provided by LVGL library. You can use parts of the code as an example or reuse it when building your own custom applications.
