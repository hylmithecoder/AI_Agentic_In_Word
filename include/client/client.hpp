#include "../../third_party/nfd/include/nfd.hpp"
#include "../debugger.hpp"
// #include <socketapi.h>
#include <vector>
#include <windows.h>

namespace MCPHelper {
class MCPClient {
public:
  const string socket = "ws://localhost:9910";
  bool isConnected = false;
  MCPClient() { ConnectToMCP(); }
  ~MCPClient() {}

  bool ConnectToMCP();

  vector<string> getFilePath();
  // template<typename T>
  void SendPrompt(const int &id, const string &prompt, const string &filePath);

  // Helper function
  wstring StringToWstring(const string &str);
};
} // namespace MCPHelper