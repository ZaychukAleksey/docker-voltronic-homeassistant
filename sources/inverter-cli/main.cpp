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
  std::cout
      << "\nUSAGE:  ./inverter_poller <args> [-r <command>], [-h | --help], [-1 | --run-once]\n\n"
         "SUPPORTED ARGUMENTS:\n"
         "    -r <raw-command>      TX 'raw' command to the inverter\n"
         "    --crc                 Append CRC to the raw command\n"
         "    -h | --help           This Help Message\n"
         "    -1 | --run-once       Runs one iteration on the inverter, and then exits\n"
         "    -c                    Optional path to the configuration file (default: ./inverter.conf)"
         "    -d                    Additional debugging\n\n"
         "RAW COMMAND EXAMPLES (see protocol manual for complete list):\n"
         "Set output source priority  POP00     (Utility first)\n"
         "                            POP01     (Solar first)\n"
         "                            POP02     (SBU)\n"
         "Set charger priority        PCP00     (Utility first)\n"
         "                            PCP01     (Solar first)\n"
         "                            PCP02     (Solar and utility)\n"
         "                            PCP03     (Solar only)\n"
         "Set other commands          PEa / PDa (Enable/disable buzzer)\n"
         "                            PEb / PDb (Enable/disable overload bypass)\n"
         "                            PEj / PDj (Enable/disable power saving)\n"
         "                            PEu / PDu (Enable/disable overload restart)\n"
         "                            PEx / PDx (Enable/disable backlight)\n\n";
}

const std::string& GetConfigurationFileName(const CommandLineArguments& cmd_args) {
  static const std::string kConfigurationFile = "./inverter.conf";
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
  auto rated_info = adapter.GetRatedInformation();
  auto warnings = adapter.GetWarnings();
  auto info = adapter.GetGeneralInfo();

  // There appears to be a discrepancy in actual DMM measured current vs what the meter is
  // telling me it's getting, so lets add a variable we can multiply/divide by to adjust if
  // needed.  This should be set in the config so it can be changed without program recompile.
  auto pv_input_current = info.pv_input_voltage > 0
                          ? info.pv_input_power / info.pv_input_voltage
                          : 0.f;
//  const auto pv_input_current = info.pv_input_current * settings.amperage_factor;

  // It appears on further inspection of the documentation, that the input current is actually
  // current that is going out to the battery at battery voltage (NOT at PV voltage).  This
  // would explain the larger discrepancy we saw before.
//  const auto pv_input_watts = (info.battery_voltage_from_scc * pv_input_current) * settings.watt_factor;

  printf("{\n");
  printf("  \"Mode\":%s,\n", DeviceModeToString(mode).data());
  printf("  \"Grid_voltage\":%.1f,\n", info.grid_voltage);
  printf("  \"Grid_frequency\":%.1f,\n", info.grid_frequency);
  printf("  \"AC_out_voltage\":%.1f,\n", info.ac_output_voltage);
  printf("  \"AC_out_frequency\":%.1f,\n", info.ac_output_frequency);
  printf("  \"PV_in_voltage\":%.1f,\n", info.pv_input_voltage);
  printf("  \"PV_in_current\":%.1f,\n", pv_input_current);
  printf("  \"PV_in_watts\":%d,\n", info.pv_input_power);
  printf("  \"SCC_voltage\":%.4f,\n", info.battery_voltage_from_scc);
  printf("  \"Load_pct\":%d,\n", info.output_load_percent);
  printf("  \"Load_watt\":%d,\n", info.ac_output_active_power);
  printf("  \"Load_va\":%d,\n", info.ac_output_apparent_power);
  printf("  \"Bus_voltage\":%d,\n", (info.bus_voltage ? *info.bus_voltage : 0));
  printf("  \"Heatsink_temperature\":%d,\n", info.inverter_heat_sink_temperature);
  printf("  \"Battery_capacity\":%d,\n", info.battery_capacity);
  printf("  \"Battery_voltage\":%.2f,\n", info.battery_voltage);
  printf("  \"Battery_charge_current\":%d,\n", info.battery_charging_current);
  printf("  \"Battery_discharge_current\":%d,\n", info.battery_discharge_current);
  printf("  \"Load_status_on\":%c,\n", info.device_status[3]);
  printf("  \"SCC_charge_on\":%c,\n", info.device_status[6]);
  printf("  \"AC_charge_on\":%c,\n", info.device_status[7]);
  printf("  \"Battery_voltage_offset_for_fans_on\":%d,\n",
         info.battery_voltage_offset_for_fans_on);
  printf("  \"Eeprom_version\":%d,\n", info.eeprom_version);
  printf("  \"PV_charging_power\":%d,\n", info.pv_charging_power);
  printf("  \"Charging_to_floating_mode\":%c,\n", info.device_status_2[0]);
  printf("  \"Switch_On\":%c,\n", info.device_status_2[1]);
  printf("  \"Dustproof_installed\":%c,\n", info.device_status_2[2]);
  printf("  \"Battery_recharge_voltage\":%.1f,\n", rated_info.battery_recharge_voltage);
  printf("  \"Battery_under_voltage\":%.1f,\n", rated_info.battery_under_voltage);
  printf("  \"Battery_bulk_voltage\":%.1f,\n", rated_info.battery_bulk_voltage);
  printf("  \"Battery_float_voltage\":%.1f,\n", rated_info.battery_float_voltage);
  printf("  \"Max_grid_charge_current\":%d,\n", rated_info.current_max_ac_charging_current);
  printf("  \"Max_charge_current\":%d,\n", rated_info.current_max_charging_current);
  printf("  \"Out_source_priority\":%s,\n", OutputSourcePriorityToString(rated_info.output_source_priority).data());
  printf("  \"Charger_source_priority\":%s,\n", ChargerPriorityToString(rated_info.charger_source_priority).data());
  printf("  \"Battery_redischarge_voltage\":%.1f,\n", rated_info.battery_redischarge_voltage);
  printf("  \"Warnings\":\"%s\"\n", ConcatenateWarnings(warnings).c_str());
  printf("}\n");
}

void InitLogging(const CommandLineArguments& arguments) {
  spdlog::set_default_logger(spdlog::stderr_color_st("std err"));
  spdlog::set_pattern("[%H:%M:%S.%e %^%l%$] %v");
  if (arguments.IsSet("-d")) {
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Debug logging enabled.");
  } else {
    spdlog::set_level(spdlog::level::err);
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
