#pragma once

#include <filesystem>
#include <map>
#include <string>

struct AppConfig {
  std::string serialPort;
  int baudRate = 115200;
  std::map<std::string, std::string> mappings;
};

class Config {
public:
  static AppConfig Load(const std::filesystem::path& path);
  static bool Save(const std::filesystem::path& path, const AppConfig& config, std::string& error);
};
