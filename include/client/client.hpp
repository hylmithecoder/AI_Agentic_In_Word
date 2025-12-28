#pragma once
#include "../../third_party/nfd/include/nfd.hpp"
#include "../../third_party/nlohmann/json.hpp"
#include "../debugger.hpp"
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>
#include <winhttp.h>

using namespace std;
using namespace NFD;
using json = nlohmann::json;

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
  string SendMessageToWebsocket(const string &jsonMessage);

  // File picker
  vector<string> getFilePath();

  // Word Application integration - get current document name/path
  static void SetWordApp(IDispatch *pApp);
  static string GetCurrentWordDocument();
  static IDispatch *s_pWordApp;
  bool IsDocumentSaved();

  // Send prompt to AI
  void SendPrompt(const int &id, const string &prompt, const string &filePath,
                  const string &currentFile);

  struct historyChat {
    string message;
    string timestamp;
    string role;
  };
  vector<historyChat> historyChat;

  void SetHistoryChat();
  // Helper function
  wstring StringToWstring(const string &str);
  string WstringToString(const wstring &str);

  void WriteNonStream(const wstring &message);
};

} // namespace MCPHelper