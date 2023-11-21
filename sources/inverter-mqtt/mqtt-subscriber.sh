#!/bin/bash

# The script subscribes on COMMANDS topic and listens for commands sent from HomeAssistant.
# Then resends them to inverter_poller

CONFIG_FILE="/opt/mqtt.json"
MQTT_SERVER=`cat $CONFIG_FILE | jq '.server' -r`
MQTT_PORT=`cat $CONFIG_FILE | jq '.port' -r`
MQTT_DISCOVERY_PREFIX=`cat $CONFIG_FILE | jq '.discovery_prefix' -r`
MQTT_DEVICE_NAME=`cat $CONFIG_FILE | jq '.device_name' -r`
MQTT_SERIAL_NUMBER=`cat $CONFIG_FILE | jq '.serial_number' -r`
MQTT_USERNAME=`cat $CONFIG_FILE | jq '.username' -r`
MQTT_PASSWORD=`cat $CONFIG_FILE | jq '.password' -r`

function subscribe () {
    mosquitto_sub -h $MQTT_SERVER -p $MQTT_PORT -u "$MQTT_USERNAME" -P "$MQTT_PASSWORD" \
    -i ""$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"" -t "$MQTT_DISCOVERY_PREFIX/sensor/""$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER""/COMMANDS" -q 1
}

function reply () {
    mosquitto_pub -h $MQTT_SERVER -p $MQTT_PORT -u "$MQTT_USERNAME" -P "$MQTT_PASSWORD" -i ""$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"" -t "$MQTT_DISCOVERY_PREFIX/sensor/""$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER""/COMMANDS" -q 1 -m "$*"
}

subscribe | while read rawcmd; do
    echo "[$(date +%F+%T)] Incoming request send: [$rawcmd] to inverter."
    for attempt in $(seq 3); do
	REPLY=$(/opt/inverter_poller -r $rawcmd)
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
# done < <(mosquitto_sub -h $MQTT_SERVER -p $MQTT_PORT -u "$MQTT_USERNAME" -P "$MQTT_PASSWORD" -i ""$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"" -t "$MQTT_DISCOVERY_PREFIX/sensor/""$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER""/COMMANDS" -q 1)
