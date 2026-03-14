#pragma once

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

class SerialPort {
public:
  SerialPort();
  ~SerialPort();

  bool Connect(const std::string& portName, int baudRate, std::string& error);
  void Disconnect();

  bool IsConnected() const;
  bool SendLine(const std::string& line, std::string& error);

  static std::vector<std::string> ListPorts();

private:
#ifdef _WIN32
  HANDLE handle_;
#endif
};
