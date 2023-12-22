#include "pi30_protocol_adapter.hh"

#include "exceptions.h"
#include "spdlog/spdlog.h"


namespace {

//Generate 8 bit CRC of supplied string + 1 as used in REVO PI30 protocol
//CHK=DATE0+....+1
//CHK=The accumulated value of sent data+1, single byte.
char CHK(std::string_view s) {
  int crc = 0;
  for (char c: s) {
    crc += c;
  }
  ++crc;
  crc &= 0xFF;
  return crc;
}

BatteryType GetBatteryType(int type) {
  switch (type) {
    case 0: return BatteryType::kAgm;
    case 1: return BatteryType::kFlooded;
    case 2: return BatteryType::kUser;
    case 3: return BatteryType::kPYL;
    case 4: return BatteryType::kSH;
    default: throw std::runtime_error(fmt::format("Unknown battery type: {}", type));
  }
}

InputVoltageRange GetInputVoltageRange(int type) {
  switch (type) {
    case 0: return InputVoltageRange::kAppliance;
    case 1: return InputVoltageRange::kUps;
    default: throw std::runtime_error(fmt::format("Unknown InputVoltageRange: {}", type));
  }
}

OutputSourcePriority GetOutputSourcePriority(int type) {
  switch (type) {
    case 0: return OutputSourcePriority::kUtility;
    case 1: return OutputSourcePriority::kSolarUtilityBattery;
    case 2: return OutputSourcePriority::kSolarBatteryUtility;
    default: throw std::runtime_error(fmt::format("Unknown OutputSourcePriority: {}", type));
  }
}

ChargerPriority GetChargerPriority(int type) {
  switch (type) {
    case 0: return ChargerPriority::kUtilityFirst;
    case 1: return ChargerPriority::kSolarFirst;
    case 2: return ChargerPriority::kSolarAndUtility;
    case 3: return ChargerPriority::kOnlySolar;
    default: throw std::runtime_error(fmt::format("Unknown ChargerPriority: {}", type));
  }
}

MachineType GetMachineType(int type) {
  switch (type) {
    case 0: return MachineType::kGridTie;
    case 1: return MachineType::kGridTie;
    default: throw std::runtime_error(fmt::format("Unknown MachineType: {}", type));
  }
}

Topology GetTopology(int type) {
  switch (type) {
    case 0: return Topology::kTransformless;
    case 1: return Topology::kTransformer;
    default: throw std::runtime_error(fmt::format("Unknown Topology: {}", type));
  }
}

OutputMode GetOutputMode(int type) {
  switch (type) {
    case 0: return OutputMode::kSingle;
    case 1: return OutputMode::kParallel;
    case 2: return OutputMode::kPhase1Of3;
    case 3: return OutputMode::kPhase2Of3;
    case 4: return OutputMode::kPhase3Of3;
    default: throw std::runtime_error(fmt::format("Unknown OutputMode: {}", type));
  }
}

}  // namespace

DeviceMode Pi30ProtocolAdapter::GetMode() {
  // Expected response: "M"
  const auto mode = GetDeviceModeRaw();
  if (mode == "P") return DeviceMode::kPowerOn;
  if (mode == "S") return DeviceMode::kStandby;
  if (mode == "Y") return DeviceMode::kBypass;
  if (mode == "L") return DeviceMode::kLine;
  if (mode == "B") return DeviceMode::kBattery;
  if (mode == "T") return DeviceMode::kBatteryTest;
  if (mode == "F") return DeviceMode::kFault;
  if (mode == "H") return DeviceMode::kPowerSaving;
  if (mode == "D") return DeviceMode::kShutdown;
  if (mode == "G") return DeviceMode::kGrid;
  if (mode == "C") return DeviceMode::kCharge;
  if (mode == "E") return DeviceMode::kEco;
  throw std::runtime_error(fmt::format("Unknown device mode: {}", mode));
}

RatedInformation Pi30ProtocolAdapter::GetRatedInfo() {
  auto str = GetDeviceRatingInformationRaw();
  if (str.length() < 80) {
    // Too short reply. Probably it's something like InfiniSolarE5.5KW, which returns the following:
    // BBB.B FF.F III.I EEE.E DDD.D AA.A GGG.G R MM T
    throw UnsupportedProtocolException("unknown");
  }

  // MG.MGX
  // BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PPP QQQ O P Q R SS T U VV.V W X YYY Z

  // HS_MS_MSX
  // BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X

  int battery_type, input_voltage_range, output_source_priority, charger_source_priority,
      machine_type, topology, output_mode;
  RatedInformation result;
  const auto n_args = sscanf(str.c_str(),
                             "%f %f %f %f %f %d %d %f %f %f %f %f %1d %d %d %1d %1d %1d %1d %2d %1d %1d %f",
                             &result.grid_rating_voltage,
                             &result.grid_rating_current,
                             &result.ac_output_rating_voltage,
                             &result.ac_output_rating_frequency,
                             &result.ac_output_rating_current,
                             &result.ac_output_rating_apparent_power,
                             &result.ac_output_rating_active_power,
                             &result.battery_nominal_voltage,
                             &result.battery_stop_discharging_voltage_with_grid,
                             &result.battery_under_voltage,
                             &result.battery_bulk_voltage,
                             &result.battery_float_voltage,
                             &battery_type,
                             &result.max_ac_charging_current,
                             &result.max_charging_current,
                             &input_voltage_range,
                             &output_source_priority,
                             &charger_source_priority,
                             &result.parallel_max_num,
                             &machine_type,
                             &topology,
                             &output_mode,
                             &result.battery_stop_charging_voltage_with_grid
                             // PV OK condition for parallel
                             // PV power balance
                             // Max. charging time at C.V stage
                             // Operation Logic
                             // Max discharging current
                             );
  if (n_args < 23) {
    throw std::runtime_error("Unexpected data in GetRatingInformation: " + str);
  }

  result.battery_type = GetBatteryType(battery_type);
  result.input_voltage_range = GetInputVoltageRange(input_voltage_range);
  result.output_source_priority = GetOutputSourcePriority(output_source_priority);
  result.charger_source_priority = GetChargerPriority(charger_source_priority);
  result.machine_type = GetMachineType(machine_type);
  result.topology = GetTopology(machine_type);
  result.output_mode = GetOutputMode(output_mode);
  return result;
}

// TODO: print human-readable warnings. The problem is that different documents define completely
//  different tables for them.
std::vector<std::string> Pi30ProtocolAdapter::GetWarnings() {
  auto str = GetDeviceWarningStatusRaw();
  for (auto c : str) {
    if (c == 1) {
      // Something is wrong.
      return {str};
    }
  }
  // No warnings detected.
  return {};
}

StatusInfo Pi30ProtocolAdapter::GetStatusInfo() {
  auto str = GetDeviceGeneralStatusRaw();
  // Again, three different documents describe tree different reply structure:
  // BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EE.E UUU.U WW.WW PPPPP b7b6b5b4b3b2b1b0 QQ VV MMMMM b10b9b8 Y ZZ AAAA
  // BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EEEE UUU.U WW.WW PPPPP b7b6b5b4b3b2b1b0
  // MMM.M CBBBBB HH.H CZZZ.Z LLL.L MMMMM NN.N QQQ.Q DDD KKK.K VVV.V SSS.S RRR.R XXX PPPPP EEEEE OOOOO UUU.U WWW.W YYY.Y TTT.T b7b6b5b4b3b2b1b0a0a1
  // Here the first two will be handled.
  StatusInfo result;
  float pv_input_current;

  char device_status[10];
  const auto n_args = sscanf(str.c_str(),
                             "%f %f %f %f %4d %4d %3d %3d %f %d %3d %4d %f %f %f %5d %8s",
                             &result.grid_voltage,
                             &result.grid_frequency,
                             &result.ac_output_voltage,
                             &result.ac_output_frequency,
                             &result.ac_output_apparent_power,
                             &result.ac_output_active_power,
                             &result.output_load_percent,
                             &result.pv_bus_voltage,
                             &result.battery_voltage,
                             &result.battery_charging_current,
                             &result.battery_capacity,
                             &result.inverter_heat_sink_temperature,
                             &pv_input_current,
                             &result.pv_input_voltage,
                             &result.battery_voltage_from_scc,
                             &result.battery_discharge_current,
                             device_status
                             );
  result.pv_input_power = result.pv_input_voltage * pv_input_current;
  return result;
}

int Pi30ProtocolAdapter::GetTotalGeneratedEnergy() {
  // It seems that only InfiniSolarE5.5KW supports that.
  return 0;
}