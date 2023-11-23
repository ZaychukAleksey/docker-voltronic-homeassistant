#!/bin/bash
#
# The script invokes inverter_poller binary ONCE, parsers its JSON result and publish data into
# relevant topics.
#
CONFIG_FILE="$1"
INVERTER_POLLER_BINARY="$2"

MQTT_SERVER=`cat $CONFIG_FILE | jq '.mqtt_server' -r`
MQTT_PORT=`cat $CONFIG_FILE | jq '.mqtt_port' -r`
MQTT_USERNAME=`cat $CONFIG_FILE | jq '.mqtt_username' -r`
MQTT_PASSWORD=`cat $CONFIG_FILE | jq '.mqtt_password' -r`
MQTT_DISCOVERY_PREFIX=`cat $CONFIG_FILE | jq '.mqtt_discovery_prefix' -r`

DEVICE_NAME=`cat $CONFIG_FILE | jq '.device_name' -r`
DEVICE_SERIAL_NUMBER=`cat $CONFIG_FILE | jq '.device_serial_number' -r`
DEVICE_ID="$DEVICE_NAME""_$DEVICE_SERIAL_NUMBER"


INFLUX_ENABLED=`cat $CONFIG_FILE | jq '.influx.enabled' -r`
if [[ $INFLUX_ENABLED == "true" ]] ; then
  INFLUX_HOST=`cat $CONFIG_FILE | jq '.influx.host' -r`
  INFLUX_USERNAME=`cat $CONFIG_FILE | jq '.influx.username' -r`
  INFLUX_PASSWORD=`cat $CONFIG_FILE | jq '.influx.password' -r`
  INFLUX_DEVICE=`cat $CONFIG_FILE | jq '.influx.device' -r`
  INFLUX_PREFIX=`cat $CONFIG_FILE | jq '.influx.prefix' -r`
  INFLUX_DATABASE=`cat $CONFIG_FILE | jq '.influx.database' -r`
  INFLUX_MEASUREMENT_NAME=`cat $CONFIG_FILE | jq '.influx.namingMap.'$1'' -r`
fi

pushMQTTData() {
  CONTROL="$1"
  VALUE="$2"

  # If value is not empty
  if [ -n "$VALUE" ]; then
    mosquitto_pub -h "$MQTT_SERVER" -p "$MQTT_PORT" -u "$MQTT_USERNAME" -P "$MQTT_PASSWORD" \
        --id "$DEVICE_ID" \
        --topic "$MQTT_DISCOVERY_PREFIX/sensor/$DEVICE_ID/$CONTROL" \
        --message "$VALUE"

    if [[ $INFLUX_ENABLED == "true" ]] ; then
      pushInfluxData "$CONTROL" "$VALUE"
    fi
  fi
}

pushInfluxData () {
    curl -i -XPOST "$INFLUX_HOST/write?db=$INFLUX_DATABASE&precision=s" -u "$INFLUX_USERNAME:$INFLUX_PASSWORD" --data-binary "$INFLUX_PREFIX,device=$INFLUX_DEVICE $INFLUX_MEASUREMENT_NAME=$2"
}


###############################################################################
# Inverter modes: 1 = Power_On, 2 = Standby, 3 = Line, 4 = Battery, 5 = Fault, 6 = Power_Saving, 7 = Unknown

# timeout - Start COMMAND, and kill it if still running after DURATION.
POLLER_JSON=$(timeout 10 "$INVERTER_POLLER_BINARY" --run-once)

BASH_HASH=$(echo "$POLLER_JSON" | jq -r '. | to_entries | .[] | @sh "[\(.key)]=\(.value)"')
eval "declare -A INVERTER_DATA=($BASH_HASH)"

for key in "${!INVERTER_DATA[@]}"; do
    pushMQTTData "$key" "${INVERTER_DATA[$key]}"
done
