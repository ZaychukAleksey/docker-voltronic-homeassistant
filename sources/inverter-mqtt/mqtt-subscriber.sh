#!/bin/bash

# The script subscribes on COMMANDS topic and listens for commands sent from HomeAssistant.
# Then resends them to inverter_poller
CONFIG_FILE="$1"              # Path to configuration file
INVERTER_POLLER_BINARY="$2"   # Path to inverter_poller binary.

MQTT_SERVER=`cat $CONFIG_FILE | jq '.mqtt_server' -r`
MQTT_PORT=`cat $CONFIG_FILE | jq '.mqtt_port' -r`
MQTT_USERNAME=`cat $CONFIG_FILE | jq '.mqtt_username' -r`
MQTT_PASSWORD=`cat $CONFIG_FILE | jq '.mqtt_password' -r`
MQTT_DISCOVERY_PREFIX=`cat $CONFIG_FILE | jq '.mqtt_discovery_prefix' -r`

DEVICE_NAME=`cat $CONFIG_FILE | jq '.device_name' -r`
DEVICE_SERIAL_NUMBER=`cat $CONFIG_FILE | jq '.serial_number' -r`
DEVICE_ID="$DEVICE_NAME""_$DEVICE_SERIAL_NUMBER"

RAW_COMMANDS_TOPIC="$MQTT_DISCOVERY_PREFIX/sensor/$DEVICE_ID/COMMANDS"

function subscribe {
  mosquitto_sub -h "$MQTT_SERVER" -p "$MQTT_PORT" -u "$MQTT_USERNAME" -P "$MQTT_PASSWORD" \
      --id "$DEVICE_ID" --topic "$RAW_COMMANDS_TOPIC" -q 1
}

function reply {
  mosquitto_pub -h "$MQTT_SERVER" -p "$MQTT_PORT" -u "$MQTT_USERNAME" -P "$MQTT_PASSWORD" \
      --id "$DEVICE_ID" --topic "$RAW_COMMANDS_TOPIC" -q 1 -m "$*"
}

subscribe | while read rawcmd; do
    echo "[$(date +%F+%T)] Incoming request send: [$rawcmd] to inverter."
    for attempt in $(seq 3); do
	REPLY=$("$INVERTER_POLLER_BINARY" -r $rawcmd)
	echo "[$(date +%F+%T)] $REPLY"
        reply "[$rawcmd] [Attempt $attempt] [$REPLY]"
	[ "$REPLY" = "Reply:  ACK" ] && break
	[ "$attempt" != "3" ] && sleep 1
    done
done


# while read rawcmd;
# do
#     echo "Incoming request send: [$rawcmd] to inverter."
#     /opt/inverter-cli/bin/inverter_poller -r $rawcmd;
# done < <(mosquitto_sub -h $MQTT_SERVER -p $MQTT_PORT -u "$MQTT_USERNAME" -P "$MQTT_PASSWORD" -i ""$DEVICE_NAME"_"$DEVICE_SERIAL_NUMBER"" -t "$MQTT_DISCOVERY_PREFIX/sensor/""$DEVICE_NAME"_"$DEVICE_SERIAL_NUMBER""/COMMANDS" -q 1)
