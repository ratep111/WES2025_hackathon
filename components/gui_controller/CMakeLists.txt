idf_component_register(
    SRCS "gui_controller.c"
    INCLUDE_DIRS "."
    REQUIRES gui app-speed-estimator app-parking-sensor app-day-night-detector app-door-detector app-crash-detector sht3x-dis
)
