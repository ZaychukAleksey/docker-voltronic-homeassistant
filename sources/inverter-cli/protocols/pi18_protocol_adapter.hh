#pragma once

#include "protocol_adapter.hh"


class Pi18ProtocolAdapter : public ProtocolAdapter {
 public:
  explicit Pi18ProtocolAdapter(const SerialPort& port) : ProtocolAdapter(port) {}

  DeviceMode GetMode() override;
  GeneralInfo GetGeneralInfo() override;
  RatedInformation GetRatedInformation() override;
  std::vector<std::string> GetWarnings() override;

 protected:
  bool UseCrcInQueries() override { return true; }

  std::string GetProtocolIdRaw() { return Query("^P005PI", "^D005"); }
  std::string GetCurrentTimeRaw() { return Query("^P005PI", "^D017"); }
  std::string GetTotalGeneratedEnergyRaw() { return Query("^P005ET", "^D011"); }
  std::string GetGeneratedEnergyOfYearRaw(std::string_view year);
  std::string GetGeneratedEnergyOfMonthRaw(std::string_view year, std::string_view month);
  std::string GetGeneratedEnergyOfDayRaw(
      std::string_view year, std::string_view month, std::string_view day);
  std::string GetSeriesNumberRaw() { return Query("^P005ID", "^D025"); }
  std::string GetCpuVersionRaw() { return Query("^P006VFW", "^D020"); }
  std::string GetRatedInformationRaw() { return Query("^P007PIRI", "^D0"); }
  std::string GetGeneralStatusRaw() { return Query("^P005GS", "^D106"); }
  std::string GetWorkingModeRaw() { return Query("^P006MOD", "^D005"); }
  std::string GetFaultAndWarningStatusRaw() { return Query("^P005FWS", "^D0"); }
  std::string GetEnableDisableFlagStatusRaw() { return Query("^P007FLAG", "^D020"); }
  std::string GetDefaultValueOfChangeableParameterRaw() { return Query("^P005DI", "^D068"); }
  std::string GetMaxChargingCurrentSelectableValueRaw() { return Query("^P009MCHGCR", "^D030"); }
  std::string GetMaxAcChargingCurrentSelectableValueRaw() { return Query("^P010MUCHGCR", "^D030"); }
  // ...
  // Here could go routines to query data for parallel system, but I haven't implemented them.
  // ...
  std::string GetAcChargeTimeBucketRaw() { return Query("^P005ACCT", "^D012"); }
  std::string GetAcSupplyLoadTimeBucketRaw() { return Query("^P005ACLT", "^D012"); }
};
