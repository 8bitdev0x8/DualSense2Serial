#pragma once

#include "Config.hpp"
#include "ControllerInput.hpp"
#include "SerialPort.hpp"

#include <SDL.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

class App {
public:
  App();
  ~App();

  bool Initialize(std::string& error);
  void Run();

private:
  void Shutdown();
  void DrawUi();
  void DrawConnectionPanel();
  void DrawMappingPanel();
  void DrawStatusPanel();

  void RefreshPorts();
  void HandleControllerActions();
  void Log(const std::string& message);
  std::string MappingFor(const std::string& buttonId) const;
  void SyncMappingToConfig();

  SDL_Window* window_;
  SDL_Renderer* renderer_;
  bool running_;

  ControllerInput controller_;
  SerialPort serial_;

  std::vector<std::string> ports_;
  int selectedPortIndex_;
  int baudRate_;

  std::map<std::string, std::string> buttonMappings_;
  std::vector<std::string> logs_;
  bool autoScrollLog_;
  bool scrollLogToBottom_;

  std::filesystem::path configPath_;
  bool configDirty_;
};
