// Lightweight program to take the sensor data from a Voltronic Axpert, Mppsolar PIP, Voltacon, Effekta, and other branded OEM Inverters and send it to a MQTT server for ingestion...
// Adapted from "Maio's" C application here: https://skyboo.net/2017/03/monitoring-voltronic-power-axpert-mex-inverter-under-linux/
//
// Please feel free to adapt this code and add more parameters -- See the following forum for a breakdown on the RS323 protocol: http://forums.aeva.asn.au/viewtopic.php?t=4332
// ------------------------------------------------------------------------

#include <atomic>
#include <cstdio>
#include <filesystem>

#include <iostream>
#include <string>

#include "configuration.h"
#include "inverter.h"
#include "logging.h"
#include "main.h"

std::atomic_bool ups_status_changed(false);
std::atomic_bool ups_qmod_changed(false);
std::atomic_bool ups_qpiri_changed(false);
std::atomic_bool ups_qpigs_changed(false);
std::atomic_bool ups_qpiws_changed(false);
std::atomic_bool ups_cmd_executed(false);


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
  static std::string kConfigurationFile = "./inverter.conf";
  return cmd_args.IsSet("-c") ? cmd_args.Get("-c") : kConfigurationFile;
}

int main(int argc, char* argv[]) {
  CommandLineArguments arguments(argc, argv);
  if (arguments.IsSet("-h", "--help")) {
    PrintHelp();
    return 0;
  }
  if (arguments.IsSet("-d")) {
    SetDebugMode();
  }

  auto settings = LoadSettingsFromFile(GetConfigurationFileName(arguments));

  bool ups_status_changed(false);
  Inverter inverter(settings.device_name);
  // Logic to send 'raw commands' to the inverter.
  if (arguments.IsSet("-r")) {
    inverter.ExecuteCmd(arguments.Get("-r"));
    // We can piggyback on either GetStatus() function to return our result, it doesn't matter which
    printf("Reply:  %s\n", inverter.GetQpiriStatus().c_str());
    return 0;
  }

  const bool run_once = arguments.IsSet("-1", "--run-once");
  if (run_once) {
    inverter.Poll();
  } else {
    inverter.StartBackgroundPolling();
  }

  // Reply2
  float grid_voltage_rating;
  float grid_current_rating;
  float out_voltage_rating;
  float out_freq_rating;
  float out_current_rating;
  int out_va_rating;
  int out_watt_rating;
  float batt_rating;
  float batt_recharge_voltage;
  float batt_under_voltage;
  float batt_bulk_voltage;
  float batt_float_voltage;
  int batt_type;
  int max_grid_charge_current;
  int max_charge_current;
  int in_voltage_range;
  int out_source_priority;
  int charger_source_priority;
  int machine_type;
  int topology;
  int out_mode;
  float batt_redischarge_voltage;
  while (true) {
    dlog("DEBUG:  Start loop");
    // If inverter mode changes print it to screen

    if (ups_status_changed) {
      int mode = inverter.GetMode();
      if (mode)
        dlog("INVERTER: Mode Currently set to: %d", mode);

      ups_status_changed = false;
    }

    // Once we receive all queries print it to screen
    if (ups_qmod_changed && ups_qpiri_changed && ups_qpigs_changed) {
      ups_qmod_changed = false;
      ups_qpiri_changed = false;
      ups_qpigs_changed = false;

      int mode = inverter.GetMode();
      const auto qpigs = inverter.GetQpigsStatus();
      const auto reply2 = inverter.GetQpiriStatus();
      const auto warnings = inverter.GetWarnings();


      char parallel_max_num; //QPIRI
      sscanf(reply2.c_str(),
             "%f %f %f %f %f %d %d %f %f %f %f %f %d %d %d %d %d %d %c %d %d %d %f",
             &grid_voltage_rating,      // ^ Grid rating voltage
             &grid_current_rating,      // ^ Grid rating current per protocol, frequency in practice
             &out_voltage_rating,       // ^ AC output rating voltage
             &out_freq_rating,          // ^ AC output rating frequency
             &out_current_rating,       // ^ AC output rating current
             &out_va_rating,            // ^ AC output rating apparent power
             &out_watt_rating,          // ^ AC output rating active power
             &batt_rating,              // ^ Battery rating voltage
             &batt_recharge_voltage,    // * Battery re-charge voltage
             &batt_under_voltage,       // * Battery under voltage
             &batt_bulk_voltage,        // * Battery bulk voltage
             &batt_float_voltage,       // * Battery float voltage
             &batt_type,                // ^ Battery type - 0 AGM, 1 Flooded, 2 User
             &max_grid_charge_current,  // * Current max AC charging current
             &max_charge_current,       // * Current max charging current
             &in_voltage_range,         // ^ Input voltage range, 0 Appliance 1 UPS
             &out_source_priority,      // * Output source priority, 0 Utility first, 1 solar first, 2 SUB first
             &charger_source_priority,  // * Charger source priority 0 Utility first, 1 solar first, 2 Solar + utility, 3 only solar charging permitted
             &parallel_max_num,         // ^ Parallel max number 0-9
             &machine_type,             // ^ Machine type 00 Grid tie, 01 Off grid, 10 hybrid
             &topology,                 // ^ Topology  0 transformerless 1 transformer
             &out_mode,                 // ^ Output mode 00: single machine output, 01: parallel output, 02: Phase 1 of 3 Phase output, 03: Phase 2 of 3 Phase output, 04: Phase 3 of 3 Phase output
             &batt_redischarge_voltage);// * Battery re-discharge voltage
      // ^ PV OK condition for parallel
      // ^ PV power balance

      // There appears to be a discrepancy in actual DMM measured current vs what the meter is
      // telling me it's getting, so lets add a variable we can multiply/divide by to adjust if
      // needed.  This should be set in the config so it can be changed without program recompile.
      dlog("INVERTER: ampfactor from config is %.2f\n", settings.amperage_factor);
      dlog("INVERTER: wattfactor from config is %.2f\n", settings.watt_factor);

      const auto pv_input_current = qpigs.pv_input_current_for_battery * settings.amperage_factor;

      // It appears on further inspection of the documentation, that the input current is actually
      // current that is going out to the battery at battery voltage (NOT at PV voltage).  This
      // would explain the larger discrepancy we saw before.
      const auto pv_input_watts = (qpigs.battery_voltage_from_scc * pv_input_current) * settings.watt_factor;

      // Calculate watt-hours generated per run interval period (given as program argument)
      // pv_input_watthour = pv_input_watts / (3600000 / runinterval);
      // load_watthour = (float)load_watt / (3600000 / runinterval);

      // Print as JSON (output is expected to be parsed by another tool...)
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
      printf("  \"Battery_recharge_voltage\":%.1f,\n", batt_recharge_voltage); // QPIRI
      printf("  \"Battery_under_voltage\":%.1f,\n", batt_under_voltage); // QPIRI
      printf("  \"Battery_bulk_voltage\":%.1f,\n", batt_bulk_voltage);  // QPIRI
      printf("  \"Battery_float_voltage\":%.1f,\n", batt_float_voltage); // QPIRI
      printf("  \"Max_grid_charge_current\":%d,\n", max_grid_charge_current); // QPIRI
      printf("  \"Max_charge_current\":%d,\n", max_charge_current);  // QPIRI
      printf("  \"Out_source_priority\":%d,\n", out_source_priority); // QPIRI
      printf("  \"Charger_source_priority\":%d,\n", charger_source_priority); // QPIRI
      printf("  \"Battery_redischarge_voltage\":%.1f,\n", batt_redischarge_voltage);  // QPIRI
      printf("  \"Warnings\":\"%s\"\n", warnings.c_str());     //
      printf("}\n");

      if (run_once) {
        dlog("INVERTER: All queries complete, exiting loop.");
        return 0;
      }
    }

    sleep(1);
  }

  // FIXME this isn't reachable.
  inverter.StopBackgroundPolling();
  return 0;
}
