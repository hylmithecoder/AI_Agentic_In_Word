#pragma once
#include "../../third_party/nfd/include/nfd.hpp"
#include "../debugger.hpp"
#include <string>
#include <vector>
#include <windows.h>
#include <winhttp.h>

using namespace std;
using namespace NFD;

namespace MCPHelper {

class MCPClient {
public:
  const wstring wsHost = L"localhost";
  const INTERNET_PORT wsPort = 9910;
  bool isConnected = false;

  MCPClient() {}
  ~MCPClient() { Disconnect(); }

  // WebSocket connection
  bool ConnectToMCP();
  void Disconnect();

  // Send JSON message and get response
  string SendMessage(const string &jsonMessage);

  // File picker
  vector<string> getFilePath();

  // Send prompt to AI
  void SendPrompt(const int &id, const string &prompt, const string &filePath);

  // Helper function
  wstring StringToWstring(const string &str);
};

} // namespace MCPHelper