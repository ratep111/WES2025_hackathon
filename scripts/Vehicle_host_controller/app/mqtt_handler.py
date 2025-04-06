import sys
import paho.mqtt.client as paho
import json

class MQTT_Handler():

    mqtt_broker_address = "192.168.160.50"
    mqtt_broker_port = 1883
    _mqtt_client = paho.Client()

    def __init__(self):
        pass

    # Connect to the mqtt broker
    @staticmethod
    def connect_mqtt_broker() -> None:
        if MQTT_Handler._mqtt_client.connect(MQTT_Handler.mqtt_broker_address, MQTT_Handler.mqtt_broker_port, 60) != 0:
            print("Couldn't connect to the mqtt broker")
            sys.exit(1)

    # Disconnect from the mqtt broker
    def disconnect_mqtt_broker() -> None:
        MQTT_Handler._mqtt_client.disconnect()

    def send_via_mqtt(data : dict, topic : str) -> None:
        json_string = json.dumps(data)
        MQTT_Handler._mqtt_client.publish(topic, json_string, 0)
    
    # Function needs to be in format "def message_handling(client, userdata, msg)..."
    def add_mqtt_receive_callback(callback_func) -> None:
        MQTT_Handler._mqtt_client.on_message = callback_func

    def subscribe_mqtt_topic(topic: str) -> None:
        MQTT_Handler._mqtt_client.subscribe(topic)
    

