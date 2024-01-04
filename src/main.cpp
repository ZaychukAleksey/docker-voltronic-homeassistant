// Lightweight program to take the sensor data from a Voltronic Axpert, Mppsolar PIP, Voltacon, Effekta, and other branded OEM Inverters and send it to a MQTT server for ingestion...
// Adapted from "Maio's" C application here: https://skyboo.net/2017/03/monitoring-voltronic-power-axpert-mex-inverter-under-linux/
//
// Please feel free to adapt this code and add more parameters -- See the following forum for a breakdown on the RS323 protocol: http://forums.aeva.asn.au/viewtopic.php?t=4332
// ------------------------------------------------------------------------

#include <cstdio>
#include <iostream>
#include <string>

#include "configuration.h"
#include "mqtt/mqtt.hh"
#include "protocols/protocol_adapter.hh"
#include "spdlog/spdlog.h"


void PrintHelp() {
  std::cout <<
"\nUSAGE:  ./inverter_poller <options>\n"
"\nOPTIONS:"
"\n    -r <raw-command>    Send 'raw' command to the inverter. Commands for a particular protocol could be found in \"documentation\" directory."
"\n    --crc               Append CRC to the raw command."
"\n    -h | --help         This Help Message."
"\n    -1 | --run-once     Poll all inverter data once, then exit."
"\n    -c                  Optional path to the configuration file (default: ./inverter.conf)."
"\n    -d                  Enable additional debug logging.\n";
}

const std::string& GetConfigurationFileName(const CommandLineArguments& cmd_args) {
  static const std::string kConfigurationFile = "inverter.conf";
  return cmd_args.IsSet("-c") ? cmd_args.Get("-c") : kConfigurationFile;
}

void InitLogging(const CommandLineArguments& arguments) {
  spdlog::set_pattern("[%H:%M:%S.%e %^%l%$] %v");
  if (arguments.IsSet("-d")) {
    spdlog::set_level(spdlog::level::debug);
  } else {
    spdlog::set_level(spdlog::level::info);
  }
}

std::unique_ptr<ProtocolAdapter> GetProtocolAdapter(SerialPort& port) {
  // TODO: save/read protocol to/from a file.
  return DetectProtocol(port);
}

int main(int argc, char* argv[]) {
  CommandLineArguments arguments(argc, argv);
  if (arguments.IsSet("-h", "--help")) {
    PrintHelp();
    return 0;
  }
  InitLogging(arguments);
  Settings::LoadFromFile(GetConfigurationFileName(arguments));
  SerialPort port(Settings::Instance().device.path);

  // Logic to send 'raw commands' to the inverter.
  if (arguments.IsSet("-r")) {
    const auto reply = port.Query(arguments.Get("-r"), arguments.IsSet("--crc"));
    printf("Reply:  %s\n", reply.c_str());
    return 0;
  }

  auto adapter = GetProtocolAdapter(port);
  const auto serial_number = adapter->GetSerialNumber();
  Settings::SetDeviceSerialNumber(serial_number);
  MqttClient::Init(Settings::Instance().mqtt, serial_number);

  const bool run_once = arguments.IsSet("-1", "--run-once");
  while (true) {
    adapter->GetMode();
    // TODO: query rated info only when changes are expected.
    adapter->GetRatedInfo();
    adapter->GetStatusInfo();
    adapter->GetWarnings();

    if (run_once) {
      break;
    }

    const auto polling_interval = Settings::Instance().polling_interval;
    spdlog::info("Wait for {} seconds before the next poll...", polling_interval);
    sleep(polling_interval);
  }

  return 0;
}
