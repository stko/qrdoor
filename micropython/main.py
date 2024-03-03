# Bibliotheken laden
from machine import UART, Pin, unique_id, reset
import network
from umqtt.simple import MQTTClient
import utime
import ujson


def connectMQTT(device_id):
    """
    does all the MQTT setup
    """
    client = MQTTClient(
        client_id=b"QRDOOR_" + device_id,
        server=settings["mqtt_server"].encode("utf-8"),
        port=settings["mqtt_port"],
        user=settings["mqtt_user"].encode("utf-8"),
        password=settings["mqtt_password"].encode("utf-8"),
        keepalive=settings["mqtt_keepalive"],
        ssl=settings["mqtt_ssl"],
        ssl_params=settings["mqtt_serial_params"],
    )
    client.connect()
    client.set_callback(subscribe)
    topic = "qrdoor/" + device_id + "/#"
    client.subscribe(topic)
    print("subscribe to topic", topic)
    return client

def subscribe(topic, msg):
    '''
    handle the received topic
    '''

    global device_id
    topic = topic.decode("utf-8")
    msg = msg.decode("utf-8")
    print("topic", topic, msg)
    if topic == "qrdoor/" + device_id + "/open":
        try:
            seconds = int(msg)
            if seconds < 6 and seconds > 0:
                set_relais(seconds)
        except:
            pass


def set_relais(seconds):
    '''
    turns on the both output for indicator led and relais output for given seconds, if less 6 secs to avoid permanent power output
    '''
    global output_switch_time_ticks, relais, led
    output_switch_time_ticks = utime.ticks_add(utime.ticks_ms(), seconds * 1000)
    relais_pin.value(1)
    led_pin.value(1)


def control_relais():
    '''
    checks the on time and switches off again when time is reached
    '''
    global output_switch_time_ticks, relais, led
    if (
        output_switch_time_ticks > 0
        and utime.ticks_diff(output_switch_time_ticks, utime.ticks_ms()) > 0
    ):
        return
    if output_switch_time_ticks > 0:
        relais_pin.value(0)
        led_pin.value(0)
        output_switch_time_ticks = 0


# reads the settings file
try:
    with open("settings.json", encoding="utf-8") as fin:
        settings = ujson.load(fin)
        print("settings loaded from file")
except:
    print("No settings found, using defaults")
    settings = {
        "wlan_ssid": "ssid",
        "wlan_password": "wifi_password",
        "mqtt_server": "mqtt_server",
        "mqtt_port": 0,
        "mqtt_user": "mqtt_user",
        "mqtt_password": "mqtt_password",
        "mqtt_keepalive": 7200,
        "mqtt_ssl": False,
        "mqtt_serial_params": {},
    }

relais_pin = Pin(22, Pin.OUT)
led_pin = Pin(17, Pin.OUT)
switch_pin = Pin(16, Pin.IN, pull=Pin.PULL_UP)
rx_pin=Pin(21)
tx_pin=Pin(20)
uart_id=1

output_switch_time_ticks = 0

# calculate the device id
s = unique_id()
device_id = ""
for b in s:
    device_id += hex(b)[2:]


# UART0 initialization
uart = UART(uart_id, baudrate=9600, tx=tx_pin, rx=rx_pin, bits=8, parity=None, stop=1)
print("UART:", uart)

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.config(pm=0xA11140)  # Diable powersave mode
wlan.connect(settings["wlan_ssid"], settings["wlan_password"])

while True:
    if wlan.status() < 0 or wlan.status() >= 3:
        break
    print("waiting for connection...")
    utime.sleep(1)

# Handle connection error
if wlan.status() != 3:
    print("wifi connection failed")
    utime.sleep(10)
    reset()
else:
    print("connected")
    status = wlan.ifconfig()
    print("ip = " + status[0])

client = connectMQTT(device_id)

# uart input buffer
line = ""

last_switch_state = False
while True:
    # read the switch
    switch_state = switch_pin.value() != 1
    if switch_state and not last_switch_state:  # raising switch event
        last_switch_state = True
        client.publish("qrdoor/" + device_id + "/button", "on")
        print("button raising edge")
    if not switch_state and last_switch_state:  # raising switch event
        client.publish("qrdoor/" + device_id + "/button", "off")
        print("button falling edge")
    last_switch_state = switch_state
    # print("switch", switch_state)
    control_relais()
    client.check_msg()
    rxData = uart.readline()
    if rxData:
        char = rxData.decode("utf-8")
        if char == "\r":
            print(line)
            # publish as MQTT payload
            client.publish("qrdoor/" + device_id + "/code", line)
            line = ""
        else:
            line += char
