#include "client/client.hpp"
#include "debugger.hpp"
#include <sstream>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

namespace MCPHelper {

// WebSocket connection handles
static HINTERNET hSession = NULL;
static HINTERNET hConnect = NULL;
static HINTERNET hRequest = NULL;
static HINTERNET hWebSocket = NULL;

bool MCPClient::ConnectToMCP() {
  if (isConnected && hWebSocket != NULL) {
    return true; // Already connected
  }

  // Initialize WinHTTP session
  hSession = WinHttpOpen(L"AgenticAI/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                         WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

  if (!hSession) {
    DEBUG_LOG("WinHttpOpen failed: %lu", GetLastError());
    return false;
  }

  // Connect to localhost:9910
  hConnect = WinHttpConnect(hSession, L"localhost", 9910, 0);

  if (!hConnect) {
    DEBUG_LOG("WinHttpConnect failed: %lu", GetLastError());
    WinHttpCloseHandle(hSession);
    hSession = NULL;
    return false;
  }

  // Create HTTP request for WebSocket upgrade
  hRequest =
      WinHttpOpenRequest(hConnect, L"GET", L"/", NULL, WINHTTP_NO_REFERER,
                         WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

  if (!hRequest) {
    DEBUG_LOG("WinHttpOpenRequest failed: %lu", GetLastError());
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    hConnect = NULL;
    hSession = NULL;
    return false;
  }

  // Set WebSocket upgrade option
  BOOL bResult =
      WinHttpSetOption(hRequest, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, NULL, 0);

  if (!bResult) {
    DEBUG_LOG("WinHttpSetOption for WebSocket failed: %lu", GetLastError());
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    hRequest = NULL;
    hConnect = NULL;
    hSession = NULL;
    return false;
  }

  // Send the request
  bResult = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

  if (!bResult) {
    DEBUG_LOG("WinHttpSendRequest failed: %lu", GetLastError());
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    hRequest = NULL;
    hConnect = NULL;
    hSession = NULL;
    return false;
  }

  // Receive response
  bResult = WinHttpReceiveResponse(hRequest, NULL);

  if (!bResult) {
    DEBUG_LOG("WinHttpReceiveResponse failed: %lu", GetLastError());
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    hRequest = NULL;
    hConnect = NULL;
    hSession = NULL;
    return false;
  }

  // Complete the WebSocket upgrade
  hWebSocket = WinHttpWebSocketCompleteUpgrade(hRequest, NULL);

  if (!hWebSocket) {
    DEBUG_LOG("WinHttpWebSocketCompleteUpgrade failed: %lu", GetLastError());
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    hRequest = NULL;
    hConnect = NULL;
    hSession = NULL;
    return false;
  }

  // Close the request handle (no longer needed)
  WinHttpCloseHandle(hRequest);
  hRequest = NULL;

  isConnected = true;
  DEBUG_LOG("WebSocket connected to ws://localhost:9910");

  // Send health check
  string healthJson = "{\"id\":\"1\",\"type\":\"health\"}";
  DWORD dwError = WinHttpWebSocketSend(
      hWebSocket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
      (PVOID)healthJson.c_str(), (DWORD)healthJson.length());

  if (dwError != ERROR_SUCCESS) {
    DEBUG_LOG("Health check send failed: %lu", dwError);
  } else {
    DEBUG_LOG("Health check sent successfully");

    // Receive response
    char recvBuffer[4096];
    DWORD dwBytesRead = 0;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;

    dwError =
        WinHttpWebSocketReceive(hWebSocket, recvBuffer, sizeof(recvBuffer) - 1,
                                &dwBytesRead, &bufferType);

    if (dwError == ERROR_SUCCESS) {
      recvBuffer[dwBytesRead] = '\0';
      string response = string(recvBuffer);
      wstring responseW = StringToWstring(response);
      MSGBOX_INFO(L"[MCPHandler]", L"Health check response: " + responseW);
      DEBUG_LOG("Health check response: %s", recvBuffer);
    } else {
      DEBUG_LOG("Health check receive failed: %lu", dwError);
    }
  }

  return true;
}

void MCPClient::Disconnect() {
  if (hWebSocket) {
    WinHttpWebSocketClose(hWebSocket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS,
                          NULL, 0);
    WinHttpCloseHandle(hWebSocket);
    hWebSocket = NULL;
  }
  if (hConnect) {
    WinHttpCloseHandle(hConnect);
    hConnect = NULL;
  }
  if (hSession) {
    WinHttpCloseHandle(hSession);
    hSession = NULL;
  }
  isConnected = false;
}

string MCPClient::SendMessage(const string &jsonMessage) {
  if (!isConnected || !hWebSocket) {
    MSGBOX_ERROR(L"Not connected to WebSocket", L"Agentic Extension");
    return "{\"error\":\"Not connected\"}";
  }

  // Send message
  DWORD dwError = WinHttpWebSocketSend(
      hWebSocket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
      (PVOID)jsonMessage.c_str(), (DWORD)jsonMessage.length());

  if (dwError != ERROR_SUCCESS) {
    MSGBOX_ERROR(L"WebSocket send failed", L"Agentic Extension");
    return "{\"error\":\"Send failed\"}";
  }

  // Receive response
  char recvBuffer[65536];
  DWORD dwBytesRead = 0;
  WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;

  dwError =
      WinHttpWebSocketReceive(hWebSocket, recvBuffer, sizeof(recvBuffer) - 1,
                              &dwBytesRead, &bufferType);

  if (dwError != ERROR_SUCCESS) {
    MSGBOX_ERRORF(L"Agentic Extension", L"WebSocket receive failed: %lu",
                  dwError);
    DEBUG_LOG("WebSocket receive failed: %lu", dwError);
    return "{\"error\":\"Receive failed\"}";
  }

  recvBuffer[dwBytesRead] = '\0';
  return string(recvBuffer);
}

void MCPClient::SendPrompt(const int &id, const string &prompt,
                           const string &filePath) {
  if (!isConnected) {
    ConnectToMCP();
  }

  // Build JSON request
  std::stringstream ss;
  ss << "{\"id\":\"" << id << "\",\"type\":\"analyze\",\"prompt\":\"";

  // Escape prompt for JSON
  for (char c : prompt) {
    switch (c) {
    case '"':
      ss << "\\\"";
      break;
    case '\\':
      ss << "\\\\";
      break;
    case '\n':
      ss << "\\n";
      break;
    case '\r':
      ss << "\\r";
      break;
    case '\t':
      ss << "\\t";
      break;
    default:
      ss << c;
      break;
    }
  }

  ss << "\",\"file_path\":\"";

  // Escape file path for JSON
  for (char c : filePath) {
    switch (c) {
    case '"':
      ss << "\\\"";
      break;
    case '\\':
      ss << "\\\\";
      break;
    default:
      ss << c;
      break;
    }
  }

  ss << "\"}";

  string response = SendMessage(ss.str());
  MSGBOX_INFOF(L"Agentic Extension", L"SendPrompt response: %s",
               response.c_str());
}

vector<string> MCPClient::getFilePath() {
  vector<string> currentPath;
  UniquePathSet outPaths;

  // prepare filters for the dialog
  nfdfilteritem_t filterItem[2] = {{"Source code", "c,cpp,cc"},
                                   {"Headers", "h,hpp"}};

  // show the dialog
  nfdresult_t result = OpenDialogMultiple(outPaths, filterItem, 2);
  if (result == NFD_OKAY) {
    cout << "Success!" << endl;

    nfdpathsetsize_t numPaths;
    PathSet::Count(outPaths, numPaths);

    nfdpathsetsize_t i;
    for (i = 0; i < numPaths; ++i) {
      UniquePathSetPath path;
      PathSet::GetPath(outPaths, i, path);
      currentPath.push_back(path.get());
      cout << "Path " << i << ": " << path.get() << endl;
    }
    return currentPath;
  } else if (result == NFD_CANCEL) {
    cout << "User pressed cancel." << endl;
  } else {
    cout << "Error: " << GetError() << endl;
  }
  return currentPath;
}

wstring MCPClient::StringToWstring(const string &str) {
  if (str.empty())
    return L"";

  int sizeNeeded =
      MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);

  wstring wstr(sizeNeeded, 0);
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0],
                      sizeNeeded);

  return wstr;
}

void MCPClient::SetHistoryChat() {
  if (!isConnected) {
    MSGBOX_ERROR(L"Not connected to WebSocket", L"Agentic Extension");
    return;
  }

  string response = SendMessage("{\"id\":\"1\",\"type\":\"history\"}");
  ofstream his("history.json");
  if (!his.is_open()) {
    std::cerr << "Error: Unable to open the file for writing." << std::endl;
    return;
  }

  // 3. Write data to the file using the stream insertion operator (<<)
  his << response << std::endl;

  // 4. Close the file stream
  // It's good practice to close the file when you're done.
  his.close();

  MSGBOX_INFO(L"Agentic Extension",
              L"SetHistoryChat response: " + StringToWstring(response));
  string responseJsonParsed = "";
  for (char responseChar : response) {
    responseJsonParsed += responseChar;
  }
  // history_chat.push_back(StringToWstring(responseJsonParsed));
}

} // namespace MCPHelper