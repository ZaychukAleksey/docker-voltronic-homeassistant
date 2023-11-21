#!/bin/bash

CONFIG_FILE="/opt/mqtt.json"

INFLUX_ENABLED=`cat $CONFIG_FILE | jq '.influx.enabled' -r`

MQTT_SERVER=`cat $CONFIG_FILE | jq '.server' -r`
MQTT_PORT=`cat $CONFIG_FILE | jq '.port' -r`
MQTT_DISCOVERY_PREFIX=`cat $CONFIG_FILE | jq '.discovery_prefix' -r`
MQTT_DEVICE_NAME=`cat $CONFIG_FILE | jq '.device_name' -r`
MQTT_SERIAL=`cat $CONFIG_FILE | jq '.serial' -r`
MQTT_USERNAME=`cat $CONFIG_FILE | jq '.username' -r`
MQTT_PASSWORD=`cat $CONFIG_FILE | jq '.password' -r`


if [[ $INFLUX_ENABLED == "true" ]] ; then
    INFLUX_HOST=`cat $CONFIG_FILE | jq '.influx.host' -r`
    INFLUX_USERNAME=`cat $CONFIG_FILE | jq '.influx.username' -r`
    INFLUX_PASSWORD=`cat $CONFIG_FILE | jq '.influx.password' -r`
    INFLUX_DEVICE=`cat $CONFIG_FILE | jq '.influx.device' -r`
    INFLUX_PREFIX=`cat $CONFIG_FILE | jq '.influx.prefix' -r`
    INFLUX_DATABASE=`cat $CONFIG_FILE | jq '.influx.database' -r`
    INFLUX_MEASUREMENT_NAME=`cat $CONFIG_FILE | jq '.influx.namingMap.'$1'' -r`
fi

pushMQTTData () {
    if [ -n "$2" ]; then
        mosquitto_pub \
            -h $MQTT_SERVER \
            -p $MQTT_PORT \
            -u "$MQTT_USERNAME" \
            -P "$MQTT_PASSWORD" \
            -i ""$MQTT_DEVICE_NAME"_"$MQTT_SERIAL"" \
            -t "$MQTT_DISCOVERY_PREFIX/sensor/"$MQTT_DEVICE_NAME"_"$MQTT_SERIAL"/$1" \
            -m "$2"
    
        if [[ $INFLUX_ENABLED == "true" ]] ; then
            pushInfluxData "$1" "$2"
        fi
    fi
}

pushInfluxData () {
    curl -i -XPOST "$INFLUX_HOST/write?db=$INFLUX_DATABASE&precision=s" -u "$INFLUX_USERNAME:$INFLUX_PASSWORD" --data-binary "$INFLUX_PREFIX,device=$INFLUX_DEVICE $INFLUX_MEASUREMENT_NAME=$2"
}

###############################################################################
# Inverter modes: 1 = Power_On, 2 = Standby, 3 = Line, 4 = Battery, 5 = Fault, 6 = Power_Saving, 7 = Unknown

POLLER_JSON=$(timeout 10 /opt/inverter_poller --run-once)
BASH_HASH=$(echo $POLLER_JSON | jq -r '. | to_entries | .[] | @sh "[\(.key)]=\(.value)"')
eval "declare -A INVERTER_DATA=($BASH_HASH)"

for key in "${!INVERTER_DATA[@]}"; do
    pushMQTTData "$key" "${INVERTER_DATA[$key]}"
done
