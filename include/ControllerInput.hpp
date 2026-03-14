#pragma once

#include <SDL.h>

#include <array>
#include <string>
#include <utility>
#include <vector>

class ControllerInput {
public:
  ControllerInput();
  ~ControllerInput();

  void HandleEvent(const SDL_Event& event);
  std::vector<SDL_GameControllerButton> PollPressedButtons();

  bool IsConnected() const;
  std::string DeviceName() const;

  static const std::vector<std::pair<SDL_GameControllerButton, std::string>>& ButtonCatalog();
  static std::string ButtonId(SDL_GameControllerButton button);

private:
  void TryOpenController(int deviceIndex);
  void CloseController();

  SDL_GameController* controller_;
  SDL_JoystickID joystickId_;
  std::array<bool, SDL_CONTROLLER_BUTTON_MAX> previousButtons_;
};
