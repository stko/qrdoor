# Bibliotheken laden
from machine import UART, Pin, unique_id, reset
import sys
import network
from umqtt.robust2 import MQTTClient
import utime
import ujson
import time

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

def subscribe(topic, msg, retain, dup ):
    '''
    handle the received topic
    '''

    global device_id, last_watchdog_rcv
    topic = topic.decode("utf-8")
    msg = msg.decode("utf-8")
    print("topic", topic, msg)
    last_watchdog_rcv=time.time()
    if topic == "qrdoor/" + device_id + "/open":
        try:
            seconds = int(msg)
            if seconds < 6 and seconds > 0:
                set_relais(seconds)
        except:
            pass
    if topic == "qrdoor/" + device_id + "/reset":
        print("MQTT Remote Reboot")
        reset() # brute force error handling..

def set_relais(seconds):
    '''
    turns on the both output for indicator led and relais output for given seconds, if less 6 secs to avoid permanent power output
    '''
    global output_switch_time_ticks, relais, led
    output_switch_time_ticks = utime.ticks_add(utime.ticks_ms(), seconds * 1000)
    relais_pin.value(1)
    led_pin.value(1)
    print("ON for relay and LED")


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
        print("OFF for relay and LED")


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
print(settings["wlan_ssid"], settings["wlan_password"])
wlan.connect(settings["wlan_ssid"], settings["wlan_password"])


# Note that this may take some time, so we need to wait
# Wait 5 sec or until connected
tmo = 50
while not wlan.isconnected():
    utime.sleep_ms(100)
    led_pin.toggle()
    tmo -= 1
    if tmo == 0:
        break

# check if the station is connected to an AP
if wlan.isconnected():
    print("=== Station Connected to WiFi \n")
    config = wlan.ifconfig()
    led_pin.value(0)
    print("IP:{0}, Network mask:{1}, Router:{2}, DNS: {3}".format(*config))
else:
    print("wifi connection failed")
    utime.sleep(10)
    led_pin.value(1)
    reset()


client = connectMQTT(device_id)

# uart input buffer
line = ""

last_switch_state = False
watchdog_interval= 6 # defines the time between two watchdog messages
watchdog_nr_of_fails = 3 # defines the trigger level, how many times a message can fail before the system resets
last_watchdog_rcv=time.time()
last_watchdog_send=0
try:
    client.publish("qrdoor/" + device_id + "/start", "1")
    while True:
        actual_time_secs=time.time()
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
        if last_watchdog_send + watchdog_interval <= actual_time_secs:
            client.publish("qrdoor/" + device_id + "/watchdog", str(actual_time_secs))
            last_watchdog_send = actual_time_secs
        if last_watchdog_rcv + watchdog_nr_of_fails * watchdog_interval < actual_time_secs:
            print("Watchdog trigger, Reboot: ")
            reset() # brute force error handling..
        control_relais()
        client.check_msg()
        rxData = uart.readline()
        if rxData:
            print("bin",rxData)
            rxData = rxData.decode("utf-8")
            print("as str\"",rxData,"\"")
            rxChars=rxData
            line += rxChars.strip() ##add wthout eol
            print("linebuffer",line)
            if rxData[-1:]== "\r": # the QRCode reader sent
                print(line)
                # publish as MQTT payload
                client.publish("qrdoor/" + device_id + "/code", line)
                line = ""

                
except Exception as ex:
    text=str(ex)
    print("System Error, Reboot: ",text)
    reset() # brute force error handling..
