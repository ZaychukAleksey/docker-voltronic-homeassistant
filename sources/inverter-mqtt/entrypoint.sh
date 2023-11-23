#!/bin/bash

CONFIG_FILE="/opt/config/mqtt.json"
INVERTER_POLLER_BINARY="/opt/inverter_poller"

# stdbuf - Run COMMAND, with modified buffering operations for its standard streams.
# i - input, o - output, e - error.
# If MODE is 'L' the corresponding stream will be line buffered. This option is invalid with standard input.
# If MODE is '0' the corresponding stream will be unbuffered.
UNBUFFER='stdbuf -i0 -oL -eL'

# Init the mqtt server.  This creates the config topics in the MQTT server
# that the MQTT integration uses to create entities in HA.

# If broker using persistence (default HA config)...
$UNBUFFER /opt/inverter-mqtt/mqtt-init.sh "$CONFIG_FILE"
# otherwise if broker not using persistence, uncomment this:
#(while :; do $UNBUFFER /opt/inverter-mqtt/mqtt-init.sh; sleep 300; done) &

# Run the MQTT subscriber process in the background (so that way we can change
# the configuration on the inverter from home assistant).
$UNBUFFER /opt/inverter-mqtt/mqtt-subscriber.sh "$CONFIG_FILE" "$INVERTER_POLLER_BINARY" &

# Push poller updates every ~10 seconds.
while :;
do
  $UNBUFFER /opt/inverter-mqtt/mqtt-push.sh "$CONFIG_FILE" "$INVERTER_POLLER_BINARY";
  sleep 7;
done
