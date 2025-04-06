from . import terminal_ux
from . import mqtt_handler

def callback_send_navigation():
    directions_dict = {
        "LEFT"     : callback_send_left_direction,
        "RIGHT"    : callback_send_right_direction,
        "STRAIGHT" : callback_send_straight_direction 
    }

    terminal_ux.Terminal_ux._print_and_execute_menu(directions_dict)

def _callback_send_universal_direction(direction : str):
    data = {
        "direction" : direction
    }
    mqtt_handler.MQTT_Handler.send_via_mqtt(data, "gps/directions")
    print(f"Sent data via MQTT of a direction {direction}")

def callback_send_left_direction():
    _callback_send_universal_direction("LEFT")

def callback_send_right_direction():
    _callback_send_universal_direction("RIGHT")

def callback_send_straight_direction():
    _callback_send_universal_direction("STRAIGHT")



