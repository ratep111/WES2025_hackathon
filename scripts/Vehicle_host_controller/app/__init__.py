from . import mqtt_handler
from . import terminal_ux
from . import actions

def start() -> None:

    # Print ASCII art
    with open("app/assets/ascii_art.txt", "r") as nice_file:
        for row in nice_file.readlines():
            print(row)

    # Connect to the MQTT
    mqtt_handler.MQTT_Handler.connect_mqtt_broker()

    initial_menu = {
        "Send direction" : actions.callback_send_navigation
    }

    continue_ux = True
    while continue_ux:
        continue_ux = terminal_ux.Terminal_ux._print_and_execute_menu(initial_menu)

