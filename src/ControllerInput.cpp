#include "ControllerInput.hpp"

#include <algorithm>
#include <limits>

namespace {
const std::vector<std::pair<SDL_GameControllerButton, std::string>> kButtons = {
    {SDL_CONTROLLER_BUTTON_A, "A / Cross"},
    {SDL_CONTROLLER_BUTTON_B, "B / Circle"},
    {SDL_CONTROLLER_BUTTON_X, "X / Square"},
    {SDL_CONTROLLER_BUTTON_Y, "Y / Triangle"},
    {SDL_CONTROLLER_BUTTON_BACK, "Create"},
    {SDL_CONTROLLER_BUTTON_GUIDE, "PS"},
    {SDL_CONTROLLER_BUTTON_START, "Options"},
    {SDL_CONTROLLER_BUTTON_LEFTSTICK, "L3"},
    {SDL_CONTROLLER_BUTTON_RIGHTSTICK, "R3"},
    {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, "L1"},
    {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, "R1"},
    {SDL_CONTROLLER_BUTTON_DPAD_UP, "DPad Up"},
    {SDL_CONTROLLER_BUTTON_DPAD_DOWN, "DPad Down"},
    {SDL_CONTROLLER_BUTTON_DPAD_LEFT, "DPad Left"},
    {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, "DPad Right"},
};
}

ControllerInput::ControllerInput()
    : controller_(nullptr), joystickId_(-1), previousButtons_{} {}

ControllerInput::~ControllerInput() {
  CloseController();
}

void ControllerInput::HandleEvent(const SDL_Event& event) {
  if (event.type == SDL_CONTROLLERDEVICEADDED) {
    if (controller_ == nullptr) {
      TryOpenController(event.cdevice.which);
    }
  }

  if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
    if (event.cdevice.which == joystickId_) {
      CloseController();
      for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        TryOpenController(i);
        if (controller_ != nullptr) {
          break;
        }
      }
    }
  }
}

std::vector<SDL_GameControllerButton> ControllerInput::PollPressedButtons() {
  std::vector<SDL_GameControllerButton> pressed;

  if (controller_ == nullptr && SDL_WasInit(SDL_INIT_GAMECONTROLLER) != 0) {
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
      TryOpenController(i);
      if (controller_ != nullptr) {
        break;
      }
    }
  }

  if (controller_ == nullptr) {
    previousButtons_.fill(false);
    return pressed;
  }

  for (const auto& [button, label] : kButtons) {
    (void)label;
    const bool down = SDL_GameControllerGetButton(controller_, button) != 0;
    const bool wasDown = previousButtons_.at(button);
    if (down && !wasDown) {
      pressed.push_back(button);
    }
    previousButtons_.at(button) = down;
  }

  return pressed;
}

bool ControllerInput::IsConnected() const {
  return controller_ != nullptr;
}

std::string ControllerInput::DeviceName() const {
  if (controller_ == nullptr) {
    return "No controller connected";
  }

  const char* name = SDL_GameControllerName(controller_);
  return name == nullptr ? "Unknown controller" : std::string(name);
}

const std::vector<std::pair<SDL_GameControllerButton, std::string>>& ControllerInput::ButtonCatalog() {
  return kButtons;
}

std::string ControllerInput::ButtonId(SDL_GameControllerButton button) {
  return std::to_string(static_cast<int>(button));
}

void ControllerInput::TryOpenController(int deviceIndex) {
  if (!SDL_IsGameController(deviceIndex)) {
    return;
  }

  SDL_GameController* candidate = SDL_GameControllerOpen(deviceIndex);
  if (candidate == nullptr) {
    return;
  }

  controller_ = candidate;
  SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller_);
  joystickId_ = SDL_JoystickInstanceID(joystick);
  previousButtons_.fill(false);
}

void ControllerInput::CloseController() {
  if (controller_ != nullptr) {
    SDL_GameControllerClose(controller_);
    controller_ = nullptr;
    joystickId_ = -1;
  }
  previousButtons_.fill(false);
}
