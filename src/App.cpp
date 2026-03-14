#include "App.hpp"

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>

#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace {
std::string CurrentTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

  std::tm localTime{};
#ifdef _WIN32
  localtime_s(&localTime, &nowTime);
#else
  localtime_r(&nowTime, &localTime);
#endif

  std::ostringstream out;
  out << std::put_time(&localTime, "%H:%M:%S");
  return out.str();
}
}

App::App()
    : window_(nullptr),
      renderer_(nullptr),
      running_(false),
      selectedPortIndex_(-1),
      baudRate_(115200),
  autoScrollLog_(true),
  scrollLogToBottom_(false),
      configPath_(std::filesystem::current_path() / "config" / "dualsense2serial.json"),
      configDirty_(false) {
}

App::~App() {
  Shutdown();
}

bool App::Initialize(std::string& error) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) {
    error = SDL_GetError();
    return false;
  }

  window_ = SDL_CreateWindow("DualSense2Serial", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1100, 720, SDL_WINDOW_RESIZABLE);
  if (window_ == nullptr) {
    error = SDL_GetError();
    return false;
  }

  renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer_ == nullptr) {
    error = SDL_GetError();
    return false;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  if (!ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_)) {
    error = "ImGui SDL2 backend initialization failed.";
    return false;
  }

  if (!ImGui_ImplSDLRenderer2_Init(renderer_)) {
    error = "ImGui SDL renderer backend initialization failed.";
    return false;
  }

  RefreshPorts();
  const AppConfig loaded = Config::Load(configPath_);
  baudRate_ = loaded.baudRate;
  buttonMappings_ = loaded.mappings;

  auto it = std::find(ports_.begin(), ports_.end(), loaded.serialPort);
  if (it != ports_.end()) {
    selectedPortIndex_ = static_cast<int>(std::distance(ports_.begin(), it));
  }

  Log("Application initialized.");
  running_ = true;
  return true;
}

void App::Run() {
  while (running_) {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      controller_.HandleEvent(event);
      if (event.type == SDL_QUIT) {
        running_ = false;
      }
    }

    HandleControllerActions();

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    DrawUi();

    ImGui::Render();
    SDL_SetRenderDrawColor(renderer_, 20, 24, 28, 255);
    SDL_RenderClear(renderer_);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
    SDL_RenderPresent(renderer_);
  }

  SyncMappingToConfig();
  AppConfig cfg;
  cfg.baudRate = baudRate_;
  cfg.mappings = buttonMappings_;
  if (selectedPortIndex_ >= 0 && selectedPortIndex_ < static_cast<int>(ports_.size())) {
    cfg.serialPort = ports_.at(selectedPortIndex_);
  }

  std::string saveError;
  if (!Config::Save(configPath_, cfg, saveError)) {
    Log("Failed to save config: " + saveError);
  }
}

void App::Shutdown() {
  serial_.Disconnect();

  if (ImGui::GetCurrentContext() != nullptr) {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
  }

  if (renderer_ != nullptr) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }

  if (window_ != nullptr) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }

  SDL_Quit();
}

void App::DrawUi() {
  ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(1060, 690), ImGuiCond_Once);

  ImGui::Begin("DualSense2Serial");

  DrawConnectionPanel();
  ImGui::Separator();
  DrawMappingPanel();
  ImGui::Separator();
  DrawStatusPanel();

  ImGui::End();
}

void App::DrawConnectionPanel() {
  ImGui::Text("Controller: %s", controller_.DeviceName().c_str());
  ImGui::SameLine();
  if (controller_.IsConnected()) {
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Connected");
  } else {
    ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Disconnected");
  }

  if (ImGui::Button("Refresh COM Ports")) {
    RefreshPorts();
  }

  if (ports_.empty()) {
    ImGui::Text("No serial ports found.");
  } else {
    std::vector<const char*> labels;
    labels.reserve(ports_.size());
    for (const std::string& p : ports_) {
      labels.push_back(p.c_str());
    }

    if (selectedPortIndex_ < 0) {
      selectedPortIndex_ = 0;
    }
    ImGui::Combo("Serial Port", &selectedPortIndex_, labels.data(), static_cast<int>(labels.size()));
  }

  ImGui::InputInt("Baud Rate", &baudRate_);
  if (baudRate_ <= 0) {
    baudRate_ = 115200;
  }

  if (!serial_.IsConnected()) {
    if (ImGui::Button("Connect Serial")) {
      if (selectedPortIndex_ < 0 || selectedPortIndex_ >= static_cast<int>(ports_.size())) {
        Log("Select a COM port first.");
      } else {
        std::string error;
        if (serial_.Connect(ports_.at(selectedPortIndex_), baudRate_, error)) {
          Log("Serial connected: " + ports_.at(selectedPortIndex_));
        } else {
          Log("Serial connect failed: " + error);
        }
      }
    }
  } else {
    if (ImGui::Button("Disconnect Serial")) {
      serial_.Disconnect();
      Log("Serial disconnected.");
    }
  }
}

void App::DrawMappingPanel() {
  ImGui::Text("Button to Serial Mapping (message sent on button press)");
  ImGui::BeginChild("mapping_table", ImVec2(0, 320), true);

  for (const auto& [button, label] : ControllerInput::ButtonCatalog()) {
    const std::string buttonId = ControllerInput::ButtonId(button);
    std::string& current = buttonMappings_[buttonId];

    char buffer[128]{};
    std::snprintf(buffer, sizeof(buffer), "%s", current.c_str());

    ImGui::PushID(buttonId.c_str());
    ImGui::Text("%s", label.c_str());
    ImGui::SameLine(230);
    if (ImGui::InputText("##msg", buffer, sizeof(buffer))) {
      current = buffer;
      configDirty_ = true;
    }
    ImGui::PopID();
  }

  ImGui::EndChild();

  if (ImGui::Button("Save Config")) {
    SyncMappingToConfig();
    AppConfig cfg;
    cfg.baudRate = baudRate_;
    cfg.mappings = buttonMappings_;
    if (selectedPortIndex_ >= 0 && selectedPortIndex_ < static_cast<int>(ports_.size())) {
      cfg.serialPort = ports_.at(selectedPortIndex_);
    }

    std::string error;
    if (Config::Save(configPath_, cfg, error)) {
      configDirty_ = false;
      Log("Config saved.");
    } else {
      Log("Config save failed: " + error);
    }
  }
}

void App::DrawStatusPanel() {
  if (configDirty_) {
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Unsaved changes");
  }

  ImGui::Text("Status Log");
  ImGui::SameLine();
  if (ImGui::Checkbox("Auto-scroll", &autoScrollLog_)) {
    if (autoScrollLog_) {
      scrollLogToBottom_ = true;
    }
  }

  ImGui::BeginChild("status_log", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
  for (const std::string& line : logs_) {
    ImGui::TextUnformatted(line.c_str());
  }
  if (autoScrollLog_) {
    ImGui::SetScrollY(ImGui::GetScrollMaxY());
    scrollLogToBottom_ = false;
  }
  ImGui::EndChild();
}

void App::RefreshPorts() {
  const std::string currentPort = (selectedPortIndex_ >= 0 && selectedPortIndex_ < static_cast<int>(ports_.size()))
                                      ? ports_.at(selectedPortIndex_)
                                      : "";

  ports_ = SerialPort::ListPorts();
  selectedPortIndex_ = -1;

  if (!currentPort.empty()) {
    auto it = std::find(ports_.begin(), ports_.end(), currentPort);
    if (it != ports_.end()) {
      selectedPortIndex_ = static_cast<int>(std::distance(ports_.begin(), it));
    }
  }

  if (selectedPortIndex_ < 0 && !ports_.empty()) {
    selectedPortIndex_ = 0;
  }
}

void App::HandleControllerActions() {
  const auto events = controller_.PollPressedButtons();
  for (SDL_GameControllerButton button : events) {
    const std::string buttonId = ControllerInput::ButtonId(button);
    const std::string message = MappingFor(buttonId);
    if (message.empty()) {
      continue;
    }

    if (!serial_.IsConnected()) {
      Log("Mapped button pressed but serial is not connected.");
      continue;
    }

    std::string error;
    if (serial_.SendLine(message, error)) {
      Log("Sent: " + message);
    } else {
      Log("Serial send failed: " + error);
    }
  }
}

void App::Log(const std::string& message) {
  logs_.push_back("[" + CurrentTimestamp() + "] " + message);
  if (logs_.size() > 80) {
    logs_.erase(logs_.begin());
  }
  if (autoScrollLog_) {
    scrollLogToBottom_ = true;
  }
}

std::string App::MappingFor(const std::string& buttonId) const {
  auto it = buttonMappings_.find(buttonId);
  if (it == buttonMappings_.end()) {
    return "";
  }
  return it->second;
}

void App::SyncMappingToConfig() {
  std::map<std::string, std::string> filtered;
  for (const auto& [key, value] : buttonMappings_) {
    if (!value.empty()) {
      filtered.emplace(key, value);
    }
  }
  buttonMappings_ = filtered;
}
