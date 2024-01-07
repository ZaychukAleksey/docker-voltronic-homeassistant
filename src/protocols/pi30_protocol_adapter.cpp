#include "pi30_protocol_adapter.hh"

#include <format>

#include "exceptions.h"


namespace {

const auto kCommandAccepted = "ACK";

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
    default: throw std::runtime_error(std::format("Unknown battery type: {}", type));
  }
}

/// Opposite to the previous function.
std::string GetBatteryType(BatteryType t) {
  // Note that there are several PI30 protocols, and these constants may vary very differently.
  // For that reason we support only the first three of them which are common.
  switch (t) {
    case BatteryType::kAgm: return "00";
    case BatteryType::kFlooded: return "01";
    case BatteryType::kUser: return "02";
    default: throw std::runtime_error(std::format("Unexpected BatteryType: {}", ToString(t)));
  }
}

InputVoltageRange GetInputVoltageRange(int type) {
  switch (type) {
    case 0: return InputVoltageRange::kAppliance;
    case 1: return InputVoltageRange::kUps;
    default: throw std::runtime_error(std::format("Unknown InputVoltageRange: {}", type));
  }
}

/// Opposite to the previous function.
std::string GetInputVoltageRange(InputVoltageRange r) {
  switch (r) {
    case InputVoltageRange::kAppliance: return "00";
    case InputVoltageRange::kUps: return "01";
  }
  throw std::runtime_error(std::format("Unexpected InputVoltageRange: {}", ToString(r)));
}

OutputSourcePriority GetOutputSourcePriority(int type) {
  switch (type) {
    case 0: return OutputSourcePriority::kUtility;
    case 1: return OutputSourcePriority::kSolarUtilityBattery;
    case 2: return OutputSourcePriority::kSolarBatteryUtility;
    default: throw std::runtime_error(std::format("Unknown OutputSourcePriority: {}", type));
  }
}

/// Opposite to the previous function.
std::string GetOutputSourcePriority(OutputSourcePriority p) {
  switch (p) {
    case OutputSourcePriority::kUtility: return "00";
    case OutputSourcePriority::kSolarUtilityBattery: return "01";
    case OutputSourcePriority::kSolarBatteryUtility: return "02";
  }
  throw std::runtime_error(std::format("Unexpected OutputSourcePriority: {}", ToString(p)));
}

ChargerPriority GetChargerPriority(int type) {
  switch (type) {
    case 0: return ChargerPriority::kUtilityFirst;
    case 1: return ChargerPriority::kSolarFirst;
    case 2: return ChargerPriority::kSolarAndUtility;
    case 3: return ChargerPriority::kOnlySolar;
    default: throw std::runtime_error(std::format("Unknown ChargerPriority: {}", type));
  }
}

/// Opposite to the previous function.
std::string GetChargerPriority(ChargerPriority p) {
  switch (p) {
    case ChargerPriority::kUtilityFirst: return "00";
    case ChargerPriority::kSolarFirst: return "01";
    case ChargerPriority::kSolarAndUtility: return "02";
    case ChargerPriority::kOnlySolar: return "03";
  }
  throw std::runtime_error(std::format("Unexpected ChargerPriority: {}", ToString(p)));
}

MachineType GetMachineType(int type) {
  switch (type) {
    case 0: return MachineType::kGridTie;
    case 1: return MachineType::kGridTie;
    default: throw std::runtime_error(std::format("Unknown MachineType: {}", type));
  }
}

Topology GetTopology(int type) {
  switch (type) {
    case 0: return Topology::kTransformless;
    case 1: return Topology::kTransformer;
    default: throw std::runtime_error(std::format("Unknown Topology: {}", type));
  }
}

OutputMode GetOutputMode(int type) {
  switch (type) {
    case 0: return OutputMode::kSingle;
    case 1: return OutputMode::kParallel;
    case 2: return OutputMode::kPhase1Of3;
    case 3: return OutputMode::kPhase2Of3;
    case 4: return OutputMode::kPhase3Of3;
    default: throw std::runtime_error(std::format("Unknown OutputMode: {}", type));
  }
}

constexpr DeviceMode GetDeviceMode(std::string_view mode) {
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
  throw std::runtime_error(std::format("Unknown device mode: {}", mode));
}

}  // namespace

Pi30ProtocolAdapter::Pi30ProtocolAdapter(const SerialPort& port)
    : ProtocolAdapter(port) {}

void Pi30ProtocolAdapter::GetRatedInfo() {
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
      ac_output_rating_apparent_power, ac_output_rating_active_power, machine_type, topology,
      output_mode, max_ac_charging_current, max_charging_current, parallel_max_num;
  float grid_rating_voltage, grid_rating_current, ac_output_rating_voltage,
      ac_output_rating_frequency, ac_output_rating_current, battery_nominal_voltage,
      battery_stop_discharging_voltage_with_grid, battery_stop_charging_voltage_with_grid,
      battery_under_voltage, battery_bulk_voltage, battery_float_voltage;
  const auto n_args = sscanf(str.c_str(),
                             "%f %f %f %f %f %d %d %f %f %f %f %f %1d %d %d %1d %1d %1d %1d %2d %1d %1d %f",
                             &grid_rating_voltage,
                             &grid_rating_current,
                             &ac_output_rating_voltage,
                             &ac_output_rating_frequency,
                             &ac_output_rating_current,
                             &ac_output_rating_apparent_power,
                             &ac_output_rating_active_power,
                             &battery_nominal_voltage,
                             &battery_stop_discharging_voltage_with_grid,
                             &battery_under_voltage,
                             &battery_bulk_voltage,
                             &battery_float_voltage,
                             &battery_type,
                             &max_ac_charging_current,
                             &max_charging_current,
                             &input_voltage_range,
                             &output_source_priority,
                             &charger_source_priority,
                             &parallel_max_num,
                             &machine_type,
                             &topology,
                             &output_mode,
                             &battery_stop_charging_voltage_with_grid
                             // PV OK condition for parallel
                             // PV power balance
                             // Max. charging time at C.V stage
                             // Operation Logic
                             // Max discharging current
                             );
  if (n_args < 23) {
    throw std::runtime_error("Unexpected data in GetRatingInformation: " + str);
  }

  battery_nominal_voltage_.Update(battery_nominal_voltage);
  battery_stop_discharging_voltage_with_grid_.Update(battery_stop_discharging_voltage_with_grid);
  battery_stop_charging_voltage_with_grid_.Update(battery_stop_charging_voltage_with_grid);
  battery_under_voltage_.Update(battery_under_voltage);
  battery_bulk_voltage_.Update(battery_bulk_voltage);
  battery_float_voltage_.Update(battery_float_voltage);
  battery_type_.Update(GetBatteryType(battery_type));

  output_source_priority_.Update(GetOutputSourcePriority(output_source_priority));
  charger_source_priority_.Update(GetChargerPriority(charger_source_priority));
}

/* TODO: fix
// TODO: print human-readable warnings. The problem is that different documents define completely
//  different tables for them.
void Pi30ProtocolAdapter::GetWarnings() {
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
 */

void Pi30ProtocolAdapter::GetStatusInfo() {
  auto str = GetDeviceGeneralStatusRaw();
  // Again, three different documents describe tree different reply structure:
  // BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EE.E UUU.U WW.WW PPPPP b7b6b5b4b3b2b1b0 QQ VV MMMMM b10b9b8 Y ZZ AAAA
  // BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EEEE UUU.U WW.WW PPPPP b7b6b5b4b3b2b1b0
  // MMM.M CBBBBB HH.H CZZZ.Z LLL.L MMMMM NN.N QQQ.Q DDD KKK.K VVV.V SSS.S RRR.R XXX PPPPP EEEEE OOOOO UUU.U WWW.W YYY.Y TTT.T b7b6b5b4b3b2b1b0a0a1
  // Here the first two will be handled.
  float pv_input_current, grid_voltage, grid_frequency, ac_output_voltage, ac_output_frequency,
      battery_voltage, pv_input_voltage, battery_voltage_from_scc;
  int ac_output_apparent_power, ac_output_active_power, output_load_percent, pv_bus_voltage,
      battery_charging_current, battery_capacity, inverter_heat_sink_temperature,
      battery_discharge_current;

  char device_status[10];
  const auto n_args = sscanf(str.c_str(),
                             "%f %f %f %f %4d %4d %3d %3d %f %d %3d %4d %f %f %f %5d %8s",
                             &grid_voltage,
                             &grid_frequency,
                             &ac_output_voltage,
                             &ac_output_frequency,
                             &ac_output_apparent_power,
                             &ac_output_active_power,
                             &output_load_percent,
                             &pv_bus_voltage,
                             &battery_voltage,
                             &battery_charging_current,
                             &battery_capacity,
                             &inverter_heat_sink_temperature,
                             &pv_input_current,
                             &pv_input_voltage,
                             &battery_voltage_from_scc,
                             &battery_discharge_current,
                             device_status
                             );
  grid_voltage_.Update(grid_voltage);
  grid_frequency_.Update(grid_frequency);
  ac_output_voltage_.Update(ac_output_voltage);
  ac_output_frequency_.Update(ac_output_frequency);
  ac_output_apparent_power_.Update(ac_output_apparent_power);
  ac_output_active_power_.Update(ac_output_active_power);
  output_load_percent_.Update(output_load_percent);

  battery_voltage_.Update(battery_voltage);
  battery_charging_current_.Update(battery_charging_current);
  battery_capacity_.Update(battery_capacity);
  battery_voltage_from_scc_.Update(battery_voltage_from_scc);
  battery_discharge_current_.Update(battery_discharge_current);

  pv_bus_voltage_.Update(pv_bus_voltage);
  pv_input_power_.Update(pv_input_voltage * pv_input_current);

  inverter_heat_sink_temperature_.Update(inverter_heat_sink_temperature);

  // Other status info.
  // TODO InfiniSolarE5.5KW supports total generated energy. Add it.
  mode_.Update(GetDeviceMode(GetDeviceModeRaw()));
}

bool Pi30ProtocolAdapter::SetChargerPriority(ChargerPriority p) {
  return SetParam("charger priority", p,
                  [&] { return Query(std::format("PCP{}", GetChargerPriority(p)), "("); },
                  kCommandAccepted);
}

bool Pi30ProtocolAdapter::SetOutputSourcePriority(OutputSourcePriority p) {
  return SetParam("output source priority", p,
                  [&] { return Query(std::format("POP{}", GetOutputSourcePriority(p)), "("); },
                  kCommandAccepted);
}

bool Pi30ProtocolAdapter::SetBatteryType(BatteryType t) {
  return SetParam("battery type", t,
                  [&] { return Query(std::format("POP{}", GetBatteryType(t)), "("); },
                  kCommandAccepted);
}

bool Pi30ProtocolAdapter::SetInputVoltageRange(InputVoltageRange r) {
  return SetParam("battery type", r,
                  [&] { return Query(std::format("PGR{}", GetInputVoltageRange(r)), "("); },
                  kCommandAccepted);
}
