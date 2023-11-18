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
#include "inverter.h"

#include "spdlog/spdlog.h"


void PrintHelp() {
  std::cout
      << "\nUSAGE:  ./inverter_poller <args> [-r <command>], [-h | --help], [-1 | --run-once]\n\n"
         "SUPPORTED ARGUMENTS:\n"
         "    -r <raw-command>      TX 'raw' command to the inverter\n"
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

void PrintResultInJson(const Inverter& inverter, const Settings& settings) {
  // Calculate watt-hours generated per run interval period (given as program argument)
  // pv_input_watthour = pv_input_watts / (3600000 / runinterval);
  // load_watthour = (float)load_watt / (3600000 / runinterval);

  int mode = inverter.GetMode();
  const auto qpigs = inverter.GetQpigsStatus();
  const auto qpiri = inverter.GetQpiriStatus();
  const auto warnings = inverter.GetWarnings();

  // There appears to be a discrepancy in actual DMM measured current vs what the meter is
  // telling me it's getting, so lets add a variable we can multiply/divide by to adjust if
  // needed.  This should be set in the config so it can be changed without program recompile.
  const auto pv_input_current = qpigs.pv_input_current_for_battery * settings.amperage_factor;

  // It appears on further inspection of the documentation, that the input current is actually
  // current that is going out to the battery at battery voltage (NOT at PV voltage).  This
  // would explain the larger discrepancy we saw before.
  const auto pv_input_watts = (qpigs.battery_voltage_from_scc * pv_input_current) * settings.watt_factor;

  printf("{\n");

  printf("  \"Inverter_mode\":%d,\n", mode);
  printf("  \"AC_grid_voltage\":%.1f,\n", qpigs.grid_voltage);
  printf("  \"AC_grid_frequency\":%.1f,\n", qpigs.grid_frequency);
  printf("  \"AC_out_voltage\":%.1f,\n", qpigs.ac_output_voltage);
  printf("  \"AC_out_frequency\":%.1f,\n", qpigs.ac_output_frequency);
  printf("  \"PV_in_voltage\":%.1f,\n", qpigs.pv_input_voltage);
  printf("  \"PV_in_current\":%.1f,\n", qpigs.pv_input_current_for_battery);
  printf("  \"PV_in_watts\":%.1f,\n", pv_input_watts);
  printf("  \"SCC_voltage\":%.4f,\n", qpigs.battery_voltage_from_scc);
  printf("  \"Load_pct\":%d,\n", qpigs.output_load_percent);
  printf("  \"Load_watt\":%d,\n", qpigs.ac_output_active_power);
  printf("  \"Load_va\":%d,\n", qpigs.ac_output_apparent_power);
  printf("  \"Bus_voltage\":%d,\n", qpigs.bus_voltage);
  printf("  \"Heatsink_temperature\":%d,\n", qpigs.inverter_heat_sink_temperature);
  printf("  \"Battery_capacity\":%d,\n", qpigs.battery_capacity);
  printf("  \"Battery_voltage\":%.2f,\n", qpigs.battery_voltage);
  printf("  \"Battery_charge_current\":%d,\n", qpigs.battery_charging_current);
  printf("  \"Battery_discharge_current\":%d,\n", qpigs.battery_discharge_current);
  printf("  \"Load_status_on\":%c,\n", qpigs.device_status[3]);
  printf("  \"SCC_charge_on\":%c,\n", qpigs.device_status[6]);
  printf("  \"AC_charge_on\":%c,\n", qpigs.device_status[7]);
  printf("  \"Battery_voltage_offset_for_fans_on\":%d,\n",
         qpigs.battery_voltage_offset_for_fans_on);
  printf("  \"Eeprom_version\":%d,\n", qpigs.eeprom_version);
  printf("  \"PV_charging_power\":%d,\n", qpigs.pv_charging_power);
  printf("  \"Charging_to_floating_mode\":%c,\n", qpigs.device_status_2[0]);
  printf("  \"Switch_On\":%c,\n", qpigs.device_status_2[1]);
  printf("  \"Dustproof_installed\":%c,\n", qpigs.device_status_2[2]);
  printf("  \"Battery_recharge_voltage\":%.1f,\n", qpiri.battery_recharge_voltage);
  printf("  \"Battery_under_voltage\":%.1f,\n", qpiri.battery_under_voltage);
  printf("  \"Battery_bulk_voltage\":%.1f,\n", qpiri.battery_bulk_voltage);
  printf("  \"Battery_float_voltage\":%.1f,\n", qpiri.battery_float_voltage);
  printf("  \"Max_grid_charge_current\":%d,\n", qpiri.current_max_ac_charging_current);
  printf("  \"Max_charge_current\":%d,\n", qpiri.current_max_charging_current);
  printf("  \"Out_source_priority\":%d,\n", qpiri.output_source_priority);
  printf("  \"Charger_source_priority\":%d,\n", qpiri.charger_source_priority);
  printf("  \"Battery_redischarge_voltage\":%.1f,\n", qpiri.battery_redischarge_voltage);
  printf("  \"Warnings\":\"%s\"\n", warnings.c_str());
  printf("}\n");
}

int main(int argc, char* argv[]) {
  CommandLineArguments arguments(argc, argv);
  if (arguments.IsSet("-h", "--help")) {
    PrintHelp();
    return 0;
  }
  if (arguments.IsSet("-d")) {
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Debug logging enabled.");
  } else {
    spdlog::set_level(spdlog::level::err);
  }

  auto settings = LoadSettingsFromFile(GetConfigurationFileName(arguments));
  Inverter inverter(settings.device_name);

  // Logic to send 'raw commands' to the inverter.
  if (arguments.IsSet("-r")) {
    const auto reply = inverter.Query(arguments.Get("-r"), false);
    printf("Reply:  %s\n", reply.c_str());
    return 0;
  }

  const bool run_once = arguments.IsSet("-1", "--run-once");
  while (true) {
    if (inverter.Poll()) {
      // The output is expected to be parsed by another tool.
      PrintResultInJson(inverter, settings);
    } else {
      spdlog::error("Failed to retrieve all data from inverter.");
    }

    if (run_once) {
      break;
    }

    sleep(5);
  }

  return 0;
}
