# ION Start Script Example
Note: place this in a file named sense.py

````
from sense_hat import SenseHat
import Adafruit_DHT
import time
import datetime
import time
import jwt
import paho.mqtt.client as mqtt

# Define some project-based variables to be used below. This should be the only
# block of variables that you need to edit in order to run this script

ssl_private_key_filepath = '' # The .pem file of your Pi
ssl_algorithm = 'RS256' 
root_cert_filepath = '' # The .pem file of Google
project_id = '' # The project ID on Google Cloud
gcp_location = '' # The zone where your project is deployed
registry_id = '' # The ID of your registry on Google IoT Core
device_id = '' # The ID of your Pi as set up on Google IoT Core

cur_time = datetime.datetime.utcnow()

DHT_SENSOR = Adafruit_DHT.DHT11
DHT_PIN = 4

def create_jwt():
  token = {
      'iat': cur_time,
      'exp': cur_time + datetime.timedelta(minutes=60),
      'aud': project_id
  }

  with open(ssl_private_key_filepath, 'r') as f:
    private_key = f.read()

  return jwt.encode(token, private_key, ssl_algorithm)

_CLIENT_ID = 'projects/{}/locations/{}/registries/{}/devices/{}'.format(project_id, gcp_location, registry_id, device_id)
_MQTT_TOPIC = '/devices/{}/events'.format(device_id)

client = mqtt.Client(client_id=_CLIENT_ID)
client.username_pw_set(
    username='unused',
    password=create_jwt())def error_str(rc):
    return '{}: {}'.format(rc, mqtt.error_string(rc))

def on_connect(unusued_client, unused_userdata, unused_flags, rc):
    print('on_connect', error_str(rc))

def on_publish(unused_client, unused_userdata, unused_mid):
    print('on_publish')

client.on_connect = on_connect
client.on_publish = on_publish

client.tls_set(ca_certs=root_cert_filepath)
client.connect('mqtt.googleapis.com', 8883)
client.loop_start()

# Could set this granularity to whatever we want based on device, monitoring needs, etc
temperature = 0
humidity = 0
pressure = 0

sense = SenseHat()

while True:
  cur_temp = sense.get_temperature()
  cur_pressure = sense.get_pressure()
  cur_humidity = sense.get_humidity()
  if cur_temp == temperature and cur_humidity == humidity and cur_pressure == pressure:
    time.sleep(1)
    continue
  temperature = cur_temp
  pressure = cur_pressure
  humidity = cur_humidity

  payload = '{{ "ts": {}, "temperature": {}, "pressure": {}, "humidity": {} }}'.format(cur_time, "%.1f C" % temperature,"%.2f Millibars" %  press$

  client.publish(_MQTT_TOPIC, payload, qos=1)

  print("{}\n".format(payload))

  sense.set_rotation(180) # Set LED matrix to scroll from right to left

  sense.show_message("%.1f C" % temperature, scroll_speed=0.10, text_colour=[0, 255, 0]) # Show the temperature on the LED Matrix

  time.sleep(10)

  ````
