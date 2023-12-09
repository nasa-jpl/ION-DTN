# ION Start Script Example
Note: place this in a file named iot.py

````
import os
import time
from google.cloud import pubsub_v1
subscriber = pubsub_v1.SubscriberClient()
# The `subscription_path` method creates a fully qualified identifier
# in the form `projects/{project_id}/subscriptions/{subscription_name}`
subscription_path = subscriber.subscription_path(
  'ID_OF_YOUR_GOOGLE_CLOUD_PROJECT', 'ID_OF_YOUR_SUBSCRIPTION')
def callback(message):
  value = message.data
  os.system(f'echo "{value}" | bpsource ipn:2.1')
  print('Received message: {}'.format(value))
  message.ack() # Acknowledges the receipt of the message and remove it from the topic queue
subscriber.subscribe(subscription_path, callback=callback)
# The subscriber is non-blocking. We must keep the main thread from
# exiting to allow it to process messages asynchronously in the background.
print('Listening for messages on {}'.format(subscription_path))
while True:
  time.sleep(60)

  ````
