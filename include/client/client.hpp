#pragma once
#include "../../third_party/nfd/include/nfd.hpp"
#include "../../third_party/nlohmann/json.hpp"
#include "../debugger.hpp"
#include <OleAuto.h>
#include <fstream>
#include <sstream>
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

  // Send prompt with streaming response
  void SendPromptWithStream(const int &id, const string &prompt,
                            const string &filePath, const string &currentFile);

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

  void Write(const wstring &message);
  void Stream(const wstring &message);
  void StreamByWords(const wstring &message);
  void StreamByLines(const wstring &message);

private:
  // Cached COM objects for better performance
  struct StreamContext {
    IDispatch *pDoc = nullptr;
    IDispatch *pRange = nullptr;
    DISPID insertAfterDispId = 0;
    bool isValid = false;
  };

  StreamContext InitStreamContext();
  void WriteWithContext(StreamContext &ctx, const wstring &text);
  void CleanupStreamContext(StreamContext &ctx);

  using StreamCallback = function<void(const string &)>;

  string ExtractJsonString(const string &json, size_t start, size_t end);
  // string SendMessageToWebsocketWithStream(const string &message,
  //                                         StreamCallback callback);
  void StreamChunked(const wstring &message);
  vector<wstring> SplitIntoChunks(const wstring &text, size_t chunkSize);
};

} // namespace MCPHelper