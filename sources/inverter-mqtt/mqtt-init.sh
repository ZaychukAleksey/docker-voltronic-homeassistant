#!/bin/bash
#
# Script for registering the MQTT topics when the container starts for the first time.
# For details, see the list of supported integrations here:
#     https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
#
CONFIG_FILE="$1"

MQTT_SERVER=`cat "$CONFIG_FILE" | jq '.mqtt_server' -r` || exit 1
MQTT_PORT=`cat "$CONFIG_FILE" | jq '.mqtt_port' -r`
MQTT_USERNAME=`cat "$CONFIG_FILE" | jq '.mqtt_username' -r`
MQTT_PASSWORD=`cat "$CONFIG_FILE" | jq '.mqtt_password' -r`
MQTT_DISCOVERY_PREFIX=`cat "$CONFIG_FILE" | jq '.mqtt_discovery_prefix' -r`

DEVICE_NAME=`cat "$CONFIG_FILE" | jq '.device_name' -r`
DEVICE_MANUFACTURER=`cat "$CONFIG_FILE" | jq '.device_manufacturer' -r`
DEVICE_MODEL=`cat "$CONFIG_FILE" | jq '.device_model' -r`
DEVICE_SERIAL_NUMBER=`cat "$CONFIG_FILE" | jq '.device_serial_number' -r`
DEVICE_ID="$DEVICE_NAME""_$DEVICE_SERIAL_NUMBER"
DEVICE_INFO_JSON="{\"ids\":\"$DEVICE_SERIAL_NUMBER\",\"mf\":\"$DEVICE_MANUFACTURER\",\"mdl\":\"$DEVICE_MODEL\",\"name\":\"$DEVICE_NAME\"}"


registerControl() {
  COMPONENT_TYPE="$1"
  CONTROL="$2"
  PAYLOAD="$3"

  # The structure of MQTT discovery topic: <discovery_prefix>/<component>/[<device>/]<control>/config
  # For example: homeassistant/sensor/invertor_sn_64234579/Grid_voltage/config
  # https://www.home-assistant.io/integrations/mqtt/#discovery-topic
  DISCOVERY_TOPIC="$MQTT_DISCOVERY_PREFIX/$COMPONENT_TYPE/$DEVICE_ID/$CONTROL/config"

  echo -e "\nDISCOVERY_TOPIC=$DISCOVERY_TOPIC"
  echo "PAYLOAD=$PAYLOAD"

  mosquitto_pub -h $MQTT_SERVER -p $MQTT_PORT -u "$MQTT_USERNAME" -P "$MQTT_PASSWORD" \
        --id "$DEVICE_ID" \
        --topic "$DISCOVERY_TOPIC" \
        --retain \
        --message "$PAYLOAD"
}

# https://www.home-assistant.io/integrations/binary_sensor.mqtt/
registerBinarySensor() {
  CONTROL="$1"
  DEVICE_CLASS="$2" # One of https://www.home-assistant.io/integrations/binary_sensor/#device-class

  CONTROL_NAME=`echo "$CONTROL" | tr _ " "` # Replace underscores "_" with whitespaces.

  PAYLOAD="{
  \"device\": $DEVICE_INFO_JSON,
  \"name\": \"$CONTROL_NAME\",
  \"device_class\": \"$DEVICE_CLASS\",
  \"payload_off\":0,
  \"payload_on\":1,
  \"state_topic\": \""$MQTT_DISCOVERY_PREFIX"/binary_sensor/$DEVICE_ID/$CONTROL\",
  \"unique_id\": \""$DEVICE_SERIAL_NUMBER"_$CONTROL\"}"

  registerControl "binary_sensor" "$CONTROL" "$PAYLOAD"
}

# $1 - control to register.
# $2 - device class. Optional. One of https://www.home-assistant.io/integrations/sensor/#device-class
# $3 - unit of measurement. Optional.
# $4 - state class. If NOT "None", the sensor is assumed to be numerical and will be displayed as a
#                   line-chart in the frontend instead of as discrete values.
#                   If not set, "measurement" will be used by this script.
#                   https://developers.home-assistant.io/docs/core/entity/sensor/#available-state-classes
# https://www.home-assistant.io/integrations/sensor.mqtt/
registerSensor() {
  CONTROL="$1"
  DEVICE_CLASS="$2"
  UNIT_OF_MEASURE="$3"
  STATE_CLASS="$4"

  if [ -z "$2" ]; then
    # If device class wasn't passed to the function - don't include it into the payload.
    DEVICE_CLASS=""
    UNIT_OF_MEASURE=""
  else
    DEVICE_CLASS="
  \"device_class\":\"$DEVICE_CLASS\","
    if [ -z "$3" ]; then
      # If measurement units wasn't passed to the function - don't include it into the payload.
      UNIT_OF_MEASURE=""
    else
      UNIT_OF_MEASURE=",
  \"unit_of_measurement\": \"$UNIT_OF_MEASURE\""
    fi
  fi

  if [ -z "$4" ]; then
    # If state class wasn't passed to the function - use "measurement"
    STATE_CLASS="measurement"
  fi

  CONTROL_NAME=`echo "$CONTROL" | tr _ " "` # Replace underscores "_" with whitespaces.
  PAYLOAD="{
  \"device\":$DEVICE_INFO_JSON,$DEVICE_CLASS
  \"name\": \"$CONTROL_NAME\",
  \"state_class\": \"$STATE_CLASS\",
  \"state_topic\": \"$MQTT_DISCOVERY_PREFIX/sensor/$DEVICE_ID/$CONTROL\",
  \"unique_id\": \""$DEVICE_SERIAL_NUMBER"_$CONTROL\"$UNIT_OF_MEASURE}"

  registerControl "sensor" "$CONTROL" "$PAYLOAD"
}

# Info about grid.
registerSensor "Grid_voltage" "voltage"
registerSensor "Grid_frequency" "frequency" "Hz"

# Info about the output.
registerSensor "Output_voltage" "voltage"
registerSensor "Output_frequency" "frequency" "Hz"
registerSensor "Output_apparent_power" "apparent_power"
registerSensor "Output_active_power" "power" "W"
registerSensor "Output_load_percent" "None" "%"

# Info about batteries
## TODO: make it https://www.home-assistant.io/integrations/select.mqtt/
registerSensor "Battery_type"
registerSensor "Battery_capacity" "battery"
registerSensor "Battery_voltage" "voltage"
registerSensor "Battery_voltage_from_SCC" "voltage"
registerSensor "Battery_voltage_from_SCC2" "voltage"
registerSensor "Battery_discharge_current" "current" "A"
registerSensor "Battery_charge_current" "current" "A"
registerSensor "Battery_nominal_voltage" "voltage"
registerSensor "Battery_under_voltage" "voltage"
registerSensor "Battery_float_voltage" "voltage"
registerSensor "Battery_bulk_voltage" "voltage"
registerSensor "Battery_stop_discharging_voltage_with_grid" "voltage"
registerSensor "Battery_stop_charging_voltage_with_grid" "voltage"

# PV (Photovoltaics, i.e. solar panels) data.
registerSensor "PV_watts" "power" "W"
registerSensor "PV2_watts" "power" "W"
registerSensor "PV_voltage" "voltage"
registerSensor "PV2_voltage" "voltage"
registerSensor "PV_bus_voltage" "voltage"

# Mode & status & priorities
## TODO: these https://www.home-assistant.io/integrations/select.mqtt/
registerSensor "Mode"
registerSensor "Out_source_priority"
registerSensor "Charger_source_priority"

# Various info.
registerSensor "Heatsink_temperature" "temperature"
registerSensor "Mptt1_charger_temperature" "temperature"
registerSensor "Mptt2_charger_temperature" "temperature"
# TODO: find out how these will be handled. Like whether the inverter accumulates warnings and
#  returns them once (and then "resets" them), or warnings aren't accumulated and shown only to
#  reflect the current situation.
#Warnings - const list of strings

# Add a separate topic so we can send raw commands from HomeAssistant back to the inverter via MQTT
# (such as changing power modes etc.).
registerSensor "COMMANDS" "" "" "None"
