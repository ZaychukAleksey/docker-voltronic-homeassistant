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

  mosquitto_pub -h "$MQTT_SERVER" -p "$MQTT_PORT" -u "$MQTT_USERNAME" -P "$MQTT_PASSWORD" \
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
# $4 - icon. Optional. https://www.home-assistant.io/docs/configuration/customizing-devices/#icon
# https://www.home-assistant.io/integrations/sensor.mqtt/
registerSensor() {
  CONTROL="$1"
  DEVICE_CLASS="$2"
  UNIT_OF_MEASURE="$3"
  ICON="$4"

  # If NOT "None", the sensor is assumed to be numerical and will be displayed as a line-chart in
  # the frontend instead of as discrete values.
  # https://developers.home-assistant.io/docs/core/entity/sensor/#available-state-classes
  STATE_CLASS="measurement"
  if [ -z "$DEVICE_CLASS" ]; then
    DEVICE_CLASS=""
    STATE_CLASS=""
  else
    DEVICE_CLASS="
  \"device_class\":\"$DEVICE_CLASS\","
    STATE_CLASS="
  \"state_class\": \"$STATE_CLASS\","
  fi

  # If measurement units wasn't passed to the function - don't include it into the payload.
  if [ -z "$UNIT_OF_MEASURE" ]; then
    UNIT_OF_MEASURE=""
  else
    UNIT_OF_MEASURE=",
  \"unit_of_measurement\": \"$UNIT_OF_MEASURE\""
  fi

  # If icon wasn't passed to the function - don't include it into the payload.
  if [ -z "$ICON" ]; then
    ICON=""
  else
    ICON="
  \"icon\": \"mdi:$ICON\","
  fi

  CONTROL_NAME=`echo "$CONTROL" | tr _ " "` # Replace underscores "_" with whitespaces.
  PAYLOAD="{
  \"device\":$DEVICE_INFO_JSON,$DEVICE_CLASS $ICON
  \"name\": \"$CONTROL_NAME\", $STATE_CLASS
  \"state_topic\": \"$MQTT_DISCOVERY_PREFIX/sensor/$DEVICE_ID/$CONTROL\",
  \"unique_id\": \""$DEVICE_SERIAL_NUMBER"_$CONTROL\"$UNIT_OF_MEASURE}"

  registerControl "sensor" "$CONTROL" "$PAYLOAD"
}

# Info about grid.
registerSensor "Grid_voltage" "voltage" "V"
registerSensor "Grid_frequency" "frequency" "Hz"

# Info about the output.
registerSensor "Output_voltage" "voltage" "V"
registerSensor "Output_frequency" "frequency" "Hz"
registerSensor "Output_apparent_power" "apparent_power" "VA"
registerSensor "Output_active_power" "power" "W"
registerSensor "Output_load_percent" "None" "%"

# Info about batteries
## TODO: make it https://www.home-assistant.io/integrations/select.mqtt/
registerSensor "Battery_type"
registerSensor "Battery_capacity" "battery" "%"
registerSensor "Battery_voltage" "voltage" "V" "current-dc"
registerSensor "Battery_voltage_from_SCC" "voltage" "V" "current-dc"
registerSensor "Battery_voltage_from_SCC2" "voltage" "V" "current-dc"
registerSensor "Battery_discharge_current" "current" "A" "current-dc"
registerSensor "Battery_charge_current" "current" "A" "current-dc"
registerSensor "Battery_nominal_voltage" "voltage" "V" "current-dc"
registerSensor "Battery_under_voltage" "voltage" "V" "current-dc"
registerSensor "Battery_float_voltage" "voltage" "V" "current-dc"
registerSensor "Battery_bulk_voltage" "voltage" "V" "current-dc"
registerSensor "Battery_stop_discharging_voltage_with_grid" "voltage" "V" "current-dc"
registerSensor "Battery_stop_charging_voltage_with_grid" "voltage" "V" "current-dc"

# PV (Photovoltaics, i.e. solar panels) data.
registerSensor "PV_watts" "power" "W"
registerSensor "PV2_watts" "power" "W"
registerSensor "PV_voltage" "voltage" "V" "current-dc"
registerSensor "PV2_voltage" "voltage" "V" "current-dc"
registerSensor "PV_bus_voltage" "voltage" "V" "current-dc"

# Mode & status & priorities
## TODO: these https://www.home-assistant.io/integrations/select.mqtt/
registerSensor "Mode"
registerSensor "Out_source_priority"
registerSensor "Charger_source_priority"

# Various info.
registerSensor "Heatsink_temperature" "temperature" "°C"
registerSensor "Mptt1_charger_temperature" "temperature" "°C"
registerSensor "Mptt2_charger_temperature" "temperature" "°C"
# TODO: find out how these will be handled. Like whether the inverter accumulates warnings and
#  returns them once (and then "resets" them), or warnings aren't accumulated and shown only to
#  reflect the current situation.
#Warnings - const list of strings

# Add a separate topic so we can send raw commands from HomeAssistant back to the inverter via MQTT
# (such as changing power modes etc.).
registerSensor "COMMANDS"
