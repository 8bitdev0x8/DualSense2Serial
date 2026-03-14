#include "SerialPort.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

SerialPort::SerialPort()
#ifdef _WIN32
    : handle_(INVALID_HANDLE_VALUE)
#endif
{
}

SerialPort::~SerialPort() {
  Disconnect();
}

bool SerialPort::Connect(const std::string& portName, int baudRate, std::string& error) {
  Disconnect();

#ifdef _WIN32
  const std::string fullPort = "\\\\.\\" + portName;
  handle_ = CreateFileA(fullPort.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (handle_ == INVALID_HANDLE_VALUE) {
    error = "Could not open " + portName;
    return false;
  }

  DCB dcb{};
  dcb.DCBlength = sizeof(DCB);
  if (!GetCommState(handle_, &dcb)) {
    error = "GetCommState failed";
    Disconnect();
    return false;
  }

  dcb.BaudRate = static_cast<DWORD>(baudRate);
  dcb.ByteSize = 8;
  dcb.StopBits = ONESTOPBIT;
  dcb.Parity = NOPARITY;
  dcb.fDtrControl = DTR_CONTROL_ENABLE;
  dcb.fRtsControl = RTS_CONTROL_ENABLE;

  if (!SetCommState(handle_, &dcb)) {
    error = "SetCommState failed";
    Disconnect();
    return false;
  }

  COMMTIMEOUTS timeouts{};
  timeouts.ReadIntervalTimeout = 10;
  timeouts.ReadTotalTimeoutConstant = 10;
  timeouts.ReadTotalTimeoutMultiplier = 1;
  timeouts.WriteTotalTimeoutConstant = 10;
  timeouts.WriteTotalTimeoutMultiplier = 1;
  SetCommTimeouts(handle_, &timeouts);

  return true;
#else
  (void)portName;
  (void)baudRate;
  error = "Serial is only implemented for Windows in this build.";
  return false;
#endif
}

void SerialPort::Disconnect() {
#ifdef _WIN32
  if (handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;
  }
#endif
}

bool SerialPort::IsConnected() const {
#ifdef _WIN32
  return handle_ != INVALID_HANDLE_VALUE;
#else
  return false;
#endif
}

bool SerialPort::SendLine(const std::string& line, std::string& error) {
#ifdef _WIN32
  if (!IsConnected()) {
    error = "Serial port is not connected.";
    return false;
  }

  const std::string payload = line + "\n";
  DWORD bytesWritten = 0;
  const BOOL ok = WriteFile(handle_, payload.data(), static_cast<DWORD>(payload.size()), &bytesWritten, nullptr);
  if (!ok || bytesWritten != payload.size()) {
    error = "WriteFile failed";
    return false;
  }
  return true;
#else
  (void)line;
  error = "Serial is only implemented for Windows in this build.";
  return false;
#endif
}

std::vector<std::string> SerialPort::ListPorts() {
  std::vector<std::string> ports;
#ifdef _WIN32
  std::array<char, 2048> target{};
  for (int i = 1; i <= 256; ++i) {
    const std::string name = "COM" + std::to_string(i);
    const DWORD chars = QueryDosDeviceA(name.c_str(), target.data(), static_cast<DWORD>(target.size()));
    if (chars > 0) {
      ports.push_back(name);
    }
  }
#endif
  return ports;
}
