#!/bin/bash
#
# Simple script to register the MQTT topics when the container starts for the first time...

CONFIG_FILE="/opt/mqtt.json"
MQTT_SERVER=`cat $CONFIG_FILE | jq '.server' -r`
MQTT_PORT=`cat $CONFIG_FILE | jq '.port' -r`
MQTT_DISCOVERY_PREFIX=`cat $CONFIG_FILE | jq '.discovery_prefix' -r`
MQTT_DEVICE_NAME=`cat $CONFIG_FILE | jq '.device_name' -r`
MQTT_MANUFACTURER=`cat $CONFIG_FILE | jq '.manufacturer' -r`
MQTT_MODEL=`cat $CONFIG_FILE | jq '.model' -r`
MQTT_SERIAL_NUMBER=`cat $CONFIG_FILE | jq '.serial_number' -r`
MQTT_VER=`cat $CONFIG_FILE | jq '.ver' -r`
MQTT_USERNAME=`cat $CONFIG_FILE | jq '.username' -r`
MQTT_PASSWORD=`cat $CONFIG_FILE | jq '.password' -r`

DEVICE_ID="$MQTT_DEVICE_NAME""_$MQTT_SERIAL_NUMBER"

registerControl() {
  CONTROL="$1"
  PAYLOAD="$2"

  # The structure of MQTT discovery topic: <discovery_prefix>/<component>/[<device>/]<control>/config
  TOPIC="$MQTT_DISCOVERY_PREFIX/sensor/$DEVICE_ID/$CONTROL/config"

  mosquitto_pub -h $MQTT_SERVER -p $MQTT_PORT -u "$MQTT_USERNAME" -P "$MQTT_PASSWORD" \
        --id "$DEVICE_ID" \
        --topic "$TOPIC" \
        --retain \
        --message "$PAYLOAD"
}

registerTopic() {
  CONTROL="$1"
  UNIT_OF_MEASURE="$2"
  MDI_ICON="$3"
  DEVICE_CLASS="$4"

  CONTROL_NAME=`echo "$CONTROL" | tr _ " "` # Replace underscores "_" with whitespaces.

  # See the list of integrations here: https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
  PAYLOAD="{
  \"name\": \"$CONTROL_NAME\",
  \"unique_id\": \""$MQTT_SERIAL_NUMBER"_$CONTROL\",
  \"device\": { \"ids\": \""$MQTT_SERIAL_NUMBER"\",
                \"mf\": \""$MQTT_MANUFACTURER"\",
                \"mdl\": \""$MQTT_MODEL"\",
                \"name\": \""$MQTT_DEVICE_NAME"\",
                \"sw\": \""$MQTT_VER"\"},
  \"state_topic\": \""$MQTT_DISCOVERY_PREFIX"/sensor/"$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"/$CONTROL\",
  \"state_class\": \"measurement\",
  \"device_class\": \"$DEVICE_CLASS\",
  \"unit_of_measurement\": \"$UNIT_OF_MEASURE\",
  \"icon\": \"mdi:$MDI_ICON\"}"

  registerControl $CONTROL $PAYLOAD
}

# Remove the energy topic because they are not good for HA. Device class:total increasing or total it will add in statistics the differences between states, and we need to make sum of states. Best cases it is useful for influxdb.
# registerEnergyTopic () {
#     mosquitto_pub \
#         -h $MQTT_SERVER \
#         -p $MQTT_PORT \
#         -u "$MQTT_USERNAME" \
#         -P "$MQTT_PASSWORD" \
#         -i ""$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"" \
#         -t "$MQTT_DISCOVERY_PREFIX/sensor/"$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"/$1/LastReset" \
#         -r \
#         -m "1970-01-01T00:00:00+00:00"

#     mosquitto_pub \
#         -h $MQTT_SERVER \
#         -p $MQTT_PORT \
#         -u "$MQTT_USERNAME" \
#         -P "$MQTT_PASSWORD" \
#         -i ""$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"" \
#         -t ""$MQTT_DISCOVERY_PREFIX"/sensor/"$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"/$1/config" \
#         -r \
#         -m "{
#             \"name\": \"$5\",
#             \"uniq_id\": \""$MQTT_SERIAL_NUMBER"_$1\",
#             \"device\": { \"ids\": \""$MQTT_SERIAL_NUMBER"\", \"mf\": \""$MQTT_MANUFACTURER"\", \"mdl\": \""$MQTT_MODEL"\", \"name\": \""$MQTT_DEVICE_NAME"\", \"sw\": \""$MQTT_VER"\"},
#             \"state_topic\": \""$MQTT_DISCOVERY_PREFIX"/sensor/"$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"/$1\",
#             \"state_class\": \"total_increasing\",
#             \"device_class\": \"$4\",
#             \"unit_of_measurement\": \"$2\",
#             \"icon\": \"mdi:$3\"
#         }"
# }

# TODO some of such topics should use binary sensor
registerModeTopic () {
  CONTROL="$1"
  UNIT_OF_MEASURE="$2"
  MDI_ICON="$3"
  DEVICE_CLASS="$4"
  CONTROL_NAME=`echo "$CONTROL" | tr _ " "` # Replace underscores "_" with whitespaces.

  PAYLOAD="{
  \"name\": \"$CONTROL_NAME\",
  \"unique_id\": \""$MQTT_SERIAL_NUMBER"_$CONTROL\",
  \"device\": { \"ids\": \""$MQTT_SERIAL_NUMBER"\", \"mf\": \""$MQTT_MANUFACTURER"\", \"mdl\": \""$MQTT_MODEL"\", \"name\": \""$MQTT_DEVICE_NAME"\", \"sw\": \""$MQTT_VER"\"},
  \"state_topic\": \""$MQTT_DISCOVERY_PREFIX"/sensor/"$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"/$CONTROL\",
  \"unit_of_measurement\": \"$UNIT_OF_MEASURE\",
  \"icon\": \"mdi:$MDI_ICON\"
}"

  registerControl $CONTROL $PAYLOAD
}
registerInverterRawCMD () {
    mosquitto_pub \
        -h $MQTT_SERVER \
        -p $MQTT_PORT \
        -u "$MQTT_USERNAME" \
        -P "$MQTT_PASSWORD" \
        -i ""$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"" \
        -t ""$MQTT_DISCOVERY_PREFIX"/sensor/"$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"/COMMANDS/config" \
        -r \
        -m "{
            \"name\": \""$MQTT_DEVICE_NAME"_COMMANDS\",
            \"uniq_id\": \""$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"\",
            \"device\": { \"ids\": \""$MQTT_SERIAL_NUMBER"\", \"mf\": \""$MQTT_MANUFACTURER"\", \"mdl\": \""$MQTT_MODEL"\", \"name\": \""$MQTT_DEVICE_NAME"\", \"sw\": \""$MQTT_VER"\"},
            \"state_topic\": \""$MQTT_DISCOVERY_PREFIX"/sensor/"$MQTT_DEVICE_NAME"_"$MQTT_SERIAL_NUMBER"/COMMANDS\"
            }"
}

# TODO review/rewrite all
#              $1code    $2unit of measure  $3mdi $4class
registerModeTopic "AC_charge_on" "" "power" "None"
registerTopic "AC_grid_frequency" "Hz" "current-ac" "frequency"
registerTopic "AC_grid_voltage" "V" "power-plug" "voltage"
registerTopic "AC_out_frequency" "Hz" "current-ac" "frequency"
registerTopic "AC_out_voltage" "V" "power-plug" "voltage"
registerTopic "Battery_bulk_voltage" "V" "current-dc" "voltage"
registerTopic "Battery_capacity" "%" "battery-outline" "battery"
registerTopic "Battery_charge_current" "A" "current-dc" "current"
registerTopic "Battery_discharge_current" "A" "current-dc" "current"
registerTopic "Battery_float_voltage" "V" "current-dc" "voltage"
registerTopic "Battery_recharge_voltage" "V" "current-dc" "voltage"
registerTopic "Battery_redischarge_voltage" "V" "battery-negative" "voltage"
registerTopic "Battery_under_voltage" "V" "current-dc" "voltage"
registerTopic "Battery_voltage" "V" "battery-outline" "voltage"
registerTopic "Battery_voltage_offset_for_fans_on" "10mV" "battery-outline" "voltage"
registerTopic "Bus_voltage" "V" "details" "voltage"
registerModeTopic "Charger_source_priority" "" "solar-power" "None"
registerModeTopic "Charging_to_floating_mode" "" "solar-power" "None"
registerModeTopic "Dustproof_installed" "" "solar-power" "None" # 1-dustproof installed,0-no dustproof
registerModeTopic "Eeprom_version" "" "power" "None"
registerTopic "Heatsink_temperature" "°C" "details" "temperature"
registerModeTopic "Load_pct" "%" "brightness-percent" "None"
registerModeTopic "Load_status_on" "" "power" "None"
registerTopic "Load_va" "VA" "chart-bell-curve" "apparent_power"
registerTopic "Load_watt" "W" "chart-bell-curve" "power"
# registerEnergyTopic "Load_watthour" "kWh" "chart-bell-curve" "energy" "Load kWatthour"
registerTopic "Max_charge_current" "A" "current-ac" "current"
registerTopic "Max_grid_charge_current" "A" "current-ac" "current"
registerModeTopic "Inverter_mode" "" "solar-power" "None" # 1 = Power_On, 2 = Standby, 3 = Line, 4 = Battery, 5 = Fault, 6 = Power_Saving, 7 = Unknown
registerModeTopic "Out_source_priority" "" "grid" "None"
registerTopic "PV_in_current" "A" "solar-panel-large" "current"
registerTopic "PV_in_voltage" "V" "solar-panel-large" "voltage"
# registerEnergyTopic "PV_in_watthour" "kWh" "solar-panel-large" "energy" "PV in kWatthour"
registerTopic "PV_in_watts" "W" "solar-panel-large" "power"
registerTopic "PV_charging_power" "W" "solar-panel-large" "power"
registerModeTopic "SCC_charge_on" "" "power" "None"
registerTopic "SCC_voltage" "V" "current-dc" "voltage"
registerModeTopic "Switch_On" "" "power" "None"
registerModeTopic "Warnings" "" "power" "None"

# Add in a separate topic so we can send raw commands from assistant back to the inverter via MQTT (such as changing power modes etc)...
registerInverterRawCMD
