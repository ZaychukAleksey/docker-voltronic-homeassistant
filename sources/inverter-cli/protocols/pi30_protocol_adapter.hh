#pragma once

#include "protocol_adapter.hh"


class Pi30ProtocolAdapter : public ProtocolAdapter {
 public:
  explicit Pi30ProtocolAdapter(const SerialPort& port) : ProtocolAdapter(port) {}

  DeviceMode GetMode() override;
  GeneralInfo GetGeneralInfo() override;
  RatedInformation GetRatedInformation() override;
  std::vector<std::string> GetWarnings() override;

 protected:
  bool UseCrcInQueries() override { return true; }

  std::string GetDeviceProtocolIdRaw() { return Query("QPI", "(PI"); }
  std::string GetSerialNumberRaw() { return Query("QID", "("); }
  std::string GetMainCpuFirmwareVersionRaw() { return Query("QVFW", "(VERFW:"); }
  std::string GetAnotherCpuFirmwareVersionRaw() { return Query("QVFW2", "(VERFW2:"); }
  std::string GetDeviceRatingInformationRaw() { return Query("QPIRI", "("); }
  std::string GetDeviceFlagStatusRaw() { return Query("QFLAG", "("); }
  std::string GetDeviceGeneralStatusRaw() { return Query("QPIGS", "("); }
  std::string GetDeviceModeRaw() { return Query("QMOD", "("); }
  std::string GetDeviceWarningStatusRaw() { return Query("QPIWS", "("); }
  std::string GetDefaultSettingValueInformationRaw() { return Query("QDI", "("); }
  std::string GetSelectableValueAboutMaxChargingCurrentRaw() { return Query("QMCHGCR", "("); }
  std::string GetSelectableValueAboutMaxUtilityChargingCurrentRaw() { return Query("QMUCHGCR", "("); }
  std::string GetDspHasBootstrapOrNotRaw() { return Query("QBOOT", "("); }
  std::string GetOutputModeRaw() { return Query("QOPM", "("); }
  // ...
  // Here could go routines to query data for parallel system, but I haven't implemented them.
  // ...
};
