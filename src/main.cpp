// Lightweight program to take the sensor data from a Voltronic Axpert, Mppsolar PIP, Voltacon, Effekta, and other branded OEM Inverters and send it to a MQTT server for ingestion...
// Adapted from "Maio's" C application here: https://skyboo.net/2017/03/monitoring-voltronic-power-axpert-mex-inverter-under-linux/
//
// Please feel free to adapt this code and add more parameters -- See the following forum for a breakdown on the RS323 protocol: http://forums.aeva.asn.au/viewtopic.php?t=4332
// ------------------------------------------------------------------------

#include <cstdio>
#include <iostream>
#include <string>
#include <unistd.h>

#include "configuration.h"
#include "protocols/protocol_adapter.hh"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"


void PrintHelp() {
  std::cout <<
"\nUSAGE:  ./inverter_poller <options>\n"
"\nOPTIONS:"
"\n    -r <raw-command>    Send 'raw' command to the inverter. Commands for a particular protocol could be found in \"documentation\" directory."
"\n    --crc               Append CRC to the raw command."
"\n    -h | --help         This Help Message."
"\n    -1 | --run-once     Poll all inverter data once, then exit."
"\n    -c                  Optional path to the configuration file (default: ./inverter.conf)."
"\n    -d                  Additional debugging.\n";
}

const std::string& GetConfigurationFileName(const CommandLineArguments& cmd_args) {
  static const std::string kConfigurationFile = "config/inverter.conf";
  return cmd_args.IsSet("-c") ? cmd_args.Get("-c") : kConfigurationFile;
}

std::string ConcatenateWarnings(const std::vector<std::string> warnings) {
  std::string result;
  for (const auto& warning : warnings) {
    if (!result.empty()) result += ';';
    result += warning;
  }
  return result;
}

void PrintResultInJson(ProtocolAdapter& adapter, const Settings& settings) {
  // Calculate watt-hours generated per run interval period (given as program argument)
  // pv_input_watthour = pv_input_watts / (3600000 / runinterval);
  // load_watthour = (float)load_watt / (3600000 / runinterval);

  auto mode = adapter.GetMode();
  auto rated_info = adapter.GetRatedInfo();
  auto status = adapter.GetStatusInfo();
  auto total_energy = adapter.GetTotalGeneratedEnergy();
  auto warnings = adapter.GetWarnings();

  // There appears to be a discrepancy in actual DMM measured current vs what the meter is
  // telling me it's getting, so lets add a variable we can multiply/divide by to adjust if
  // needed.  This should be set in the config so it can be changed without program recompile.
//  auto pv_input_current = info.pv_input_voltage > 0
//                          ? info.pv_input_power / info.pv_input_voltage
//                          : 0.f;
//  const auto pv_input_current = info.pv_input_current * settings.amperage_factor;

  // It appears on further inspection of the documentation, that the input current is actually
  // current that is going out to the battery at battery voltage (NOT at PV voltage).  This
  // would explain the larger discrepancy we saw before.
//  const auto pv_input_watts = (info.battery_voltage_from_scc * pv_input_current) * settings.watt_factor;

  printf("{\n");

  // Info about grid.
  printf("  \"Grid_voltage\":%.1f,\n", status.grid_voltage);
  printf("  \"Grid_frequency\":%.1f,\n", status.grid_frequency);

  // Info about the output.
  printf("  \"Output_voltage\":%.1f,\n", status.ac_output_voltage);
  printf("  \"Output_frequency\":%.1f,\n", status.ac_output_frequency);
  printf("  \"Output_apparent_power\":%d,\n", status.ac_output_apparent_power);
  printf("  \"Output_active_power\":%d,\n", status.ac_output_active_power);
  printf("  \"Output_load_percent\":%d,\n", status.output_load_percent);

  // Info about batteries
  printf("  \"Battery_type\":\"%s\",\n", BatteryTypeToString(rated_info.battery_type).data());
  printf("  \"Battery_capacity\":%d,\n", status.battery_capacity);
  printf("  \"Battery_voltage\":%.2f,\n", status.battery_voltage);
  printf("  \"Battery_voltage_from_SCC\":%.4f,\n", status.battery_voltage_from_scc);
  printf("  \"Battery_voltage_from_SCC2\":%.4f,\n", status.battery_voltage_from_scc2);
  printf("  \"Battery_discharge_current\":%d,\n", status.battery_discharge_current);
  printf("  \"Battery_charge_current\":%d,\n", status.battery_charging_current);

  printf("  \"Battery_nominal_voltage\":%.1f,\n", rated_info.battery_nominal_voltage);
  printf("  \"Battery_under_voltage\":%.1f,\n", rated_info.battery_under_voltage);
  printf("  \"Battery_float_voltage\":%.1f,\n", rated_info.battery_float_voltage);
  printf("  \"Battery_bulk_voltage\":%.1f,\n", rated_info.battery_bulk_voltage);
  printf("  \"Battery_stop_discharging_voltage_with_grid\":%.1f,\n", rated_info.battery_stop_discharging_voltage_with_grid);
  printf("  \"Battery_stop_charging_voltage_with_grid\":%.1f,\n", rated_info.battery_stop_charging_voltage_with_grid);

  // PV (Photovoltaics, i.e. solar panels) data.
  printf("  \"PV_watts\":%d,\n", status.pv_input_power);
  printf("  \"PV2_watts\":%d,\n", status.pv2_input_power);
  printf("  \"PV_voltage\":%.1f,\n", status.pv_input_voltage);
  printf("  \"PV2_voltage\":%.1f,\n", status.pv2_input_voltage);
  printf("  \"PV_bus_voltage\":%d,\n", status.pv_bus_voltage);
  printf("  \"PV_total_generated_energy\":%d,\n", total_energy);

  // Mode & status & priorities
  printf("  \"Mode\":\"%s\",\n", DeviceModeToString(mode).data());
  printf("  \"Out_source_priority\":\"%s\",\n", OutputSourcePriorityToString(rated_info.output_source_priority).data());
  printf("  \"Charger_source_priority\":\"%s\",\n", ChargerPriorityToString(rated_info.charger_source_priority).data());

  // Various info.
  printf("  \"Heatsink_temperature\":%d,\n", status.inverter_heat_sink_temperature);
  printf("  \"Mptt1_charger_temperature\":%d,\n", status.mptt1_charger_temperature);
  printf("  \"Mptt2_charger_temperature\":%d,\n", status.mptt2_charger_temperature);
//  printf("  \"Load_status_on\":%c,\n", info.device_status[3]);
//  printf("  \"SCC_charge_on\":%c,\n", info.device_status[6]);
//  printf("  \"AC_charge_on\":%c,\n", info.device_status[7]);
//  printf("  \"PV_charging_power\":%d,\n", info.pv_charging_power);
  printf("  \"Warnings\":\"%s\"\n", ConcatenateWarnings(warnings).c_str());
  printf("}\n");
}

void InitLogging(const CommandLineArguments& arguments) {
  spdlog::set_default_logger(spdlog::stderr_color_st("std err"));
  spdlog::set_pattern("[%H:%M:%S.%e %^%l%$] %v");
  if (arguments.IsSet("-d")) {
    spdlog::set_level(spdlog::level::debug);
  } else {
    spdlog::set_level(spdlog::level::info);
  }
}

int main(int argc, char* argv[]) {
  CommandLineArguments arguments(argc, argv);
  if (arguments.IsSet("-h", "--help")) {
    PrintHelp();
    return 0;
  }
  InitLogging(arguments);

  auto settings = LoadSettingsFromFile(GetConfigurationFileName(arguments));
  SerialPort port(settings.device_name);

  // Logic to send 'raw commands' to the inverter.
  if (arguments.IsSet("-r")) {
    const auto reply = port.Query(arguments.Get("-r"), arguments.IsSet("--crc"));
    printf("Reply:  %s\n", reply.c_str());
    return 0;
  }

  auto adapter = ProtocolAdapter::Get(settings.protocol, port);
  const bool run_once = arguments.IsSet("-1", "--run-once");
  while (true) {
    // The output is expected to be parsed by another tool.
    PrintResultInJson(*adapter, settings);

    if (run_once) {
      break;
    }

    // TODO add config option
    sleep(5);
  }

  return 0;
}
