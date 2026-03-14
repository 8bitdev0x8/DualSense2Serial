#include "App.hpp"

#include <iostream>

int main() {
  App app;
  std::string error;
  if (!app.Initialize(error)) {
    std::cerr << "Initialization failed: " << error << std::endl;
    return 1;
  }

  app.Run();
  return 0;
}
