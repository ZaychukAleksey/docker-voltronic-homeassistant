// Lightweight program to take the sensor data from a Voltronic Axpert, Mppsolar PIP, Voltacon, Effekta, and other branded OEM Inverters and send it to a MQTT server for ingestion...
// Adapted from "Maio's" C application here: https://skyboo.net/2017/03/monitoring-voltronic-power-axpert-mex-inverter-under-linux/
//
// Please feel free to adapt this code and add more parameters -- See the following forum for a breakdown on the RS323 protocol: http://forums.aeva.asn.au/viewtopic.php?t=4332
// ------------------------------------------------------------------------

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include <iostream>
#include <string>
#include <vector>

#include "configuration.h"
#include "inverter.h"
#include "logging.h"
#include "main.h"


bool debugFlag = false;
bool runOnce = false;
Inverter* ups = nullptr;
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
  return cmd_args.CmdOptionExists("-c")
         ? cmd_args.GetCmdOption("-c")
         : kConfigurationFile;
}

int main(int argc, char* argv[]) {
  // Reply1
  float voltage_grid;
  float freq_grid;
  float voltage_out;
  float freq_out;
  int load_va;
  int load_watt;
  int load_percent;
  int voltage_bus;
  float voltage_batt;
  int batt_charge_current;
  int batt_capacity;
  int temp_heatsink;
  float pv_input_current;
  float pv_input_voltage;
  float pv_input_watts;
// float pv_input_watthour;
// float load_watthour = 0;
  float scc_voltage;
  int batt_discharge_current;
  char device_status[8];
  int battery_voltage_offset_for_fans_on;
  int eeprom_version;
  int pv_charging_power;
  char device_status2[3];

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

  // Get command flag settings from the arguments (if any)
  CommandLineArguments cmdArgs(argc, argv);
  const auto& rawcmd = cmdArgs.GetCmdOption("-r");
  if (cmdArgs.CmdOptionExists("-h") || cmdArgs.CmdOptionExists("--help")) {
    PrintHelp();
    return 0;
  }
  if (cmdArgs.CmdOptionExists("-d")) {
    debugFlag = true;
  }
  if (cmdArgs.CmdOptionExists("-1") || cmdArgs.CmdOptionExists("--run-once")) {
    runOnce = true;
  }
  log("INVERTER: Debug set");

  auto settings = LoadSettingsFromFile(GetConfigurationFileName(cmdArgs));

  bool ups_status_changed(false);
  ups = new Inverter(settings.device_name);
  // Logic to send 'raw commands' to the inverter.
  if (!rawcmd.empty()) {
    ups->ExecuteCmd(rawcmd);
    // We can piggyback on either GetStatus() function to return our result, it doesn't matter which
    printf("Reply:  %s\n", ups->GetQpiriStatus().c_str());
    exit(0);
  }

  if (runOnce) {
    ups->Poll();
  } else {
    ups->RunMultiThread();
  }

  while (true) {
    log("DEBUG:  Start loop");
    // If inverter mode changes print it to screen

    if (ups_status_changed) {
      int mode = ups->GetMode();
      if (mode)
        log("INVERTER: Mode Currently set to: %d", mode);

      ups_status_changed = false;
    }

    // Once we receive all queries print it to screen
    if (ups_qmod_changed && ups_qpiri_changed && ups_qpigs_changed) {
      ups_qmod_changed = false;
      ups_qpiri_changed = false;
      ups_qpigs_changed = false;

      int mode = ups->GetMode();
      const auto reply1 = ups->GetQpigsStatus();
      const auto reply2 = ups->GetQpiriStatus();
      const auto warnings = ups->GetWarnings();

      // Parse and display values, QPIGS, * means contained in output, ^ is not included in output
      sscanf(reply1.c_str(), "%f %f %f %f %d %d %d %d %f %d %d %d %f %f %f %d %s %d %d %d %s",
             &voltage_grid,          // * Grid voltage
             &freq_grid,             // * Grid frequency
             &voltage_out,           // * AC output voltage
             &freq_out,              // * AC output frequency
             &load_va,               // * AC output apparent power (VA)
             &load_watt,             // * AC output active power (Watt)
             &load_percent,          // * Output load percent - Maximum of W% or VA%., VA% is a percent of apparent power., W% is a percent of active power.
             &voltage_bus,           // * BUS voltage
             &voltage_batt,          // * Battery voltage
             &batt_charge_current,   // * Battery charging current
             &batt_capacity,         // * Battery capacity
             &temp_heatsink,         // * Inverter heat sink temperature
             &pv_input_current,      // * PV Input current for battery.
             &pv_input_voltage,      // * PV Input voltage 1
             &scc_voltage,           // * Battery voltage from SCC (V)
             &batt_discharge_current,// * Battery discharge current
             &device_status,        //
             &battery_voltage_offset_for_fans_on,
             &eeprom_version,
             &pv_charging_power,
             &device_status2);

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
      if (debugFlag) {
        printf("INVERTER: ampfactor from config is %.2f\n", settings.amperage_factor);
        printf("INVERTER: wattfactor from config is %.2f\n", settings.watt_factor);
      }

      pv_input_current = pv_input_current * settings.amperage_factor;

      // It appears on further inspection of the documentation, that the input current is actually
      // current that is going out to the battery at battery voltage (NOT at PV voltage).  This
      // would explain the larger discrepancy we saw before.
      pv_input_watts = (scc_voltage * pv_input_current) * settings.watt_factor;

      // Calculate watt-hours generated per run interval period (given as program argument)
      // pv_input_watthour = pv_input_watts / (3600000 / runinterval);
      // load_watthour = (float)load_watt / (3600000 / runinterval);

      // Print as JSON (output is expected to be parsed by another tool...)
      printf("{\n");

      printf("  \"Inverter_mode\":%d,\n", mode);
      printf("  \"AC_grid_voltage\":%.1f,\n", voltage_grid);    // QPIGS
      printf("  \"AC_grid_frequency\":%.1f,\n", freq_grid);     // QPIGS
      printf("  \"AC_out_voltage\":%.1f,\n", voltage_out);      // QPIGS
      printf("  \"AC_out_frequency\":%.1f,\n", freq_out);       // QPIGS
      printf("  \"PV_in_voltage\":%.1f,\n", pv_input_voltage);  // QPIGS
      printf("  \"PV_in_current\":%.1f,\n", pv_input_current);  // QPIGS
      printf("  \"PV_in_watts\":%.1f,\n",
             pv_input_watts);      // = (scc_voltage * pv_input_current) * wattfactor;
      // printf("  \"PV_in_watthour\":%.4f,\n", pv_input_watthour);
      printf("  \"SCC_voltage\":%.4f,\n", scc_voltage);         // QPIGS
      printf("  \"Load_pct\":%d,\n", load_percent);             // QPIGS
      printf("  \"Load_watt\":%d,\n", load_watt);               // QPIGS
      // printf("  \"Load_watthour\":%.4f,\n", load_watthour);
      printf("  \"Load_va\":%d,\n", load_va);                   // QPIGS
      printf("  \"Bus_voltage\":%d,\n", voltage_bus);           // QPIGS
      printf("  \"Heatsink_temperature\":%d,\n", temp_heatsink);// QPIGS
      printf("  \"Battery_capacity\":%d,\n", batt_capacity);    // QPIGS
      printf("  \"Battery_voltage\":%.2f,\n", voltage_batt);    // QPIGS
      printf("  \"Battery_charge_current\":%d,\n", batt_charge_current); // QPIGS
      printf("  \"Battery_discharge_current\":%d,\n", batt_discharge_current); // QPIGS
      printf("  \"Load_status_on\":%c,\n", device_status[3]);   // QPIGS
      printf("  \"SCC_charge_on\":%c,\n", device_status[6]);    // QPIGS
      printf("  \"AC_charge_on\":%c,\n", device_status[7]);     // QPIGS
      printf("  \"Battery_voltage_offset_for_fans_on\":%d,\n",
             battery_voltage_offset_for_fans_on); // QPIGS
      printf("  \"Eeprom_version\":%d,\n", eeprom_version); // QPIGS
      printf("  \"PV_charging_power\":%d,\n", pv_charging_power); // QPIGS
      printf("  \"Charging_to_floating_mode\":%c,\n", device_status2[0]);   // QPIGS
      printf("  \"Switch_On\":%c,\n", device_status2[1]);    // QPIGS
      printf("  \"Dustproof_installed\":%c,\n", device_status2[2]);     // QPIGS
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

      if (runOnce) {
        // there is no thread -- ups->terminateThread();
        // Do once and exit instead of loop endlessly
        log("INVERTER: All queries complete, exiting loop.");
        exit(0);
      }
    }

    sleep(1);
  }

  if (ups) {
    ups->TerminateThread();
    delete ups;
  }
  return 0;
}
