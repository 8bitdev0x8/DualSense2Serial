#include "Config.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

AppConfig Config::Load(const std::filesystem::path& path) {
  AppConfig config;

  if (!std::filesystem::exists(path)) {
    return config;
  }

  std::ifstream in(path);
  if (!in) {
    return config;
  }

  nlohmann::json j;
  try {
    in >> j;
    if (j.contains("serial_port") && j["serial_port"].is_string()) {
      config.serialPort = j["serial_port"].get<std::string>();
    }
    if (j.contains("baud_rate") && j["baud_rate"].is_number_integer()) {
      config.baudRate = j["baud_rate"].get<int>();
    }
    if (j.contains("mappings") && j["mappings"].is_object()) {
      config.mappings = j["mappings"].get<std::map<std::string, std::string>>();
    }
  } catch (...) {
    return AppConfig{};
  }

  return config;
}

bool Config::Save(const std::filesystem::path& path, const AppConfig& config, std::string& error) {
  nlohmann::json j;
  j["serial_port"] = config.serialPort;
  j["baud_rate"] = config.baudRate;
  j["mappings"] = config.mappings;

  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);

  std::ofstream out(path);
  if (!out) {
    error = "Failed to open config file for writing.";
    return false;
  }

  out << j.dump(2) << std::endl;
  if (!out) {
    error = "Failed to write config file.";
    return false;
  }

  return true;
}
