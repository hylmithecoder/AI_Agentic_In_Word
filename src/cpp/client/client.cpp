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

// Static Word Application pointer
IDispatch *MCPClient::s_pWordApp = nullptr;

// Set Word Application pointer (called from Connect.cpp)
void MCPClient::SetWordApp(IDispatch *pApp) { s_pWordApp = pApp; }

// Get current Word document name/path
string MCPClient::GetCurrentWordDocument() {
  if (!s_pWordApp) {
    return "[No Word App]";
  }

  // Get ActiveDocument property from Word Application
  DISPID dispid;
  OLECHAR *szMember = (OLECHAR *)L"ActiveDocument";
  HRESULT hr = s_pWordApp->GetIDsOfNames(IID_NULL, &szMember, 1,
                                         LOCALE_USER_DEFAULT, &dispid);

  if (FAILED(hr)) {
    return "[No Active Document]";
  }

  DISPPARAMS dp = {NULL, NULL, 0, 0};
  VARIANT result;
  VariantInit(&result);

  hr = s_pWordApp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT,
                          DISPATCH_PROPERTYGET, &dp, &result, NULL, NULL);

  if (FAILED(hr) || result.vt != VT_DISPATCH || !result.pdispVal) {
    VariantClear(&result);
    return "[No Document Open]";
  }

  IDispatch *pDoc = result.pdispVal;

  // Try to get FullName first (full path), fallback to Name (just filename for
  // unsaved docs)
  szMember = (OLECHAR *)L"FullName";
  hr =
      pDoc->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT, &dispid);

  if (FAILED(hr)) {
    // Try Name property instead
    szMember = (OLECHAR *)L"Name";
    hr = pDoc->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT,
                             &dispid);
    if (FAILED(hr)) {
      pDoc->Release();
      return "[Unknown Document]";
    }
  }

  VARIANT nameResult;
  VariantInit(&nameResult);

  hr = pDoc->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                    &dp, &nameResult, NULL, NULL);

  string docName;
  if (SUCCEEDED(hr) && nameResult.vt == VT_BSTR && nameResult.bstrVal) {
    // Convert BSTR to string
    int len = WideCharToMultiByte(CP_UTF8, 0, nameResult.bstrVal, -1, NULL, 0,
                                  NULL, NULL);
    if (len > 0) {
      docName.resize(len - 1);
      WideCharToMultiByte(CP_UTF8, 0, nameResult.bstrVal, -1, &docName[0], len,
                          NULL, NULL);
    }
  } else {
    docName = "[Document]";
  }

  VariantClear(&nameResult);
  pDoc->Release();

  return docName;
}

// Check if current Word document is saved (has a file path)
bool MCPClient::IsDocumentSaved() {
  if (!s_pWordApp) {
    return false; // No Word App
  }

  // Get ActiveDocument property from Word Application
  DISPID dispid;
  OLECHAR *szMember = (OLECHAR *)L"ActiveDocument";
  HRESULT hr = s_pWordApp->GetIDsOfNames(IID_NULL, &szMember, 1,
                                         LOCALE_USER_DEFAULT, &dispid);

  if (FAILED(hr)) {
    return false; // No Active Document property
  }

  DISPPARAMS dp = {NULL, NULL, 0, 0};
  VARIANT result;
  VariantInit(&result);

  hr = s_pWordApp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT,
                          DISPATCH_PROPERTYGET, &dp, &result, NULL, NULL);

  if (FAILED(hr) || result.vt != VT_DISPATCH || !result.pdispVal) {
    VariantClear(&result);
    return false; // No Document Open
  }

  IDispatch *pDoc = result.pdispVal;

  // Get Path property - if empty, document is not saved
  szMember = (OLECHAR *)L"Path";
  hr =
      pDoc->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT, &dispid);

  if (FAILED(hr)) {
    pDoc->Release();
    return false;
  }

  VARIANT pathResult;
  VariantInit(&pathResult);

  hr = pDoc->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                    &dp, &pathResult, NULL, NULL);

  bool isSaved = false;
  if (SUCCEEDED(hr) && pathResult.vt == VT_BSTR && pathResult.bstrVal) {
    // If path has content, document is saved
    isSaved = (SysStringLen(pathResult.bstrVal) > 0);
  }

  VariantClear(&pathResult);
  pDoc->Release();

  return isSaved;
}

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
      // MSGBOX_INFO(L"Health check response: " + responseW);
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

string MCPClient::SendMessageToWebsocket(const string &jsonMessage) {
  if (!isConnected || !hWebSocket) {
    MSGBOX_ERROR(L"Not connected to WebSocket");
    return "{\"error\":\"Not connected\"}";
  }

  // Send message
  DWORD dwError = WinHttpWebSocketSend(
      hWebSocket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
      (PVOID)jsonMessage.c_str(), (DWORD)jsonMessage.length());

  if (dwError != ERROR_SUCCESS) {
    MSGBOX_ERROR(L"WebSocket send failed");
    return "{\"error\":\"Send failed\"}";
  }

  // Receive response - handle potentially large/fragmented messages
  string fullResponse;
  char recvBuffer[8192]; // Smaller buffer for each chunk
  DWORD dwBytesRead = 0;
  WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;

  // Keep receiving until we get the complete message
  do {
    dwError =
        WinHttpWebSocketReceive(hWebSocket, recvBuffer, sizeof(recvBuffer) - 1,
                                &dwBytesRead, &bufferType);

    if (dwError != ERROR_SUCCESS) {
      MSGBOX_ERRORF(L"Agentic Extension", L"Websocket receive Failed %lu",
                    dwError);
      DEBUG_LOG("WebSocket receive failed: %lu", dwError);
      return "{\"error\":\"Receive failed\"}";
    }

    // Append received data to full response
    recvBuffer[dwBytesRead] = '\0';
    fullResponse.append(recvBuffer, dwBytesRead);

    // Check if this is the final fragment
    // WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE = 0x80000002 means final text
    // frame WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE = 0x80000003 means
    // fragment
  } while (bufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE ||
           bufferType == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE);

  // MSGBOX_INFO(L"WebSocket receive " + StringToWstring(fullResponse));
  DEBUG_LOG("Received full response: %zu bytes", fullResponse.length());
  return fullResponse;
}

void MCPClient::SendPrompt(const int &id, const string &prompt,
                           const string &filePath, const string &currentFile) {
  if (!isConnected) {
    ConnectToMCP();
  }

  // Build JSON request using nlohmann/json
  json requestJson = {{"id", std::to_string(id)},
                      {"type", "analyze"},
                      {"prompt", prompt},
                      {"file_path", filePath},
                      {"current_file", currentFile}};

  string jsonRequest = requestJson.dump();
  string response = SendMessageToWebsocket(jsonRequest);

  // Parse JSON response
  try {
    json responseJson = json::parse(response);

    // Extract content field
    if (responseJson.contains("content")) {
      string content = responseJson["content"].get<string>();
      wstring wContent = StringToWstring(content);

      // Split by sentences for more natural streaming
      vector<wstring> chunks = SplitIntoChunks(wContent, 100);

      // Stream each chunk
      for (const auto &chunk : chunks) {
        Stream(chunk);
        Sleep(100); // Delay between sentences
      }
    } else if (responseJson.contains("error")) {
      string error = responseJson["error"].get<string>();
      MSGBOX_WARNING(L"Error: " + StringToWstring(error));
    } else {
      MSGBOX_WARNING(L"Unexpected response format");
    }

  } catch (json::exception &e) {
    MSGBOX_WARNING(L"Failed to parse JSON: " + StringToWstring(e.what()));
  }

  SetHistoryChat();
}

void MCPClient::SendPromptWithStream(const int &id, const string &prompt,
                                     const string &filePath,
                                     const string &currentFile) {
  if (!isConnected) {
    ConnectToMCP();
  }

  // Build JSON request with isStream: true
  json requestJson = {{"id", std::to_string(id)},
                      {"type", "explain"},
                      {"prompt", prompt},
                      {"file_path", filePath},
                      {"current_file", currentFile},
                      {"isStream", true}};

  string jsonRequest = requestJson.dump();

  // Send the request
  if (!isConnected || hWebSocket == NULL) {
    MSGBOX_ERROR(L"Not connected to WebSocket");
    return;
  }

  DWORD dwError = WinHttpWebSocketSend(
      hWebSocket, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
      (PVOID)jsonRequest.c_str(), (DWORD)jsonRequest.length());

  if (dwError != ERROR_SUCCESS) {
    MSGBOX_ERROR(L"WebSocket send failed");
    return;
  }

  DEBUG_LOG("Streaming request sent: %s", jsonRequest.c_str());

  // Receive streaming responses until complete
  string accumulatedContent;
  char recvBuffer[8192];
  DWORD dwBytesRead = 0;
  WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;
  bool streamComplete = false;

  while (!streamComplete) {
    // Receive a WebSocket message
    string fullMessage;

    do {
      dwError = WinHttpWebSocketReceive(hWebSocket, recvBuffer,
                                        sizeof(recvBuffer) - 1, &dwBytesRead,
                                        &bufferType);

      if (dwError != ERROR_SUCCESS) {
        DEBUG_LOG("WebSocket receive failed: %lu", dwError);
        MSGBOX_ERRORF(L"Agentic Extension", L"Stream receive failed %lu",
                      dwError);
        return;
      }

      recvBuffer[dwBytesRead] = '\0';
      fullMessage.append(recvBuffer, dwBytesRead);

    } while (bufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE ||
             bufferType == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE);

    // Parse the received JSON message
    try {
      json responseJson = json::parse(fullMessage);

      // Check for status field
      if (responseJson.contains("status")) {
        string status = responseJson["status"].get<string>();

        if (status == "streaming") {
          // Extract and stream the content chunk
          if (responseJson.contains("content")) {
            string contentChunk = responseJson["content"].get<string>();
            wstring wChunk = StringToWstring(contentChunk);

            // Write chunk directly to Word document
            Stream(wChunk);

            // Accumulate for history
            accumulatedContent += contentChunk;
          }
        } else if (status == "complete") {
          // Streaming is complete
          streamComplete = true;
          DEBUG_LOG("Stream completed");
        } else if (status == "error") {
          // Handle error
          string errorMsg = responseJson.contains("content")
                                ? responseJson["content"].get<string>()
                                : "Unknown error";
          MSGBOX_WARNING(L"Streaming error: " + StringToWstring(errorMsg));
          streamComplete = true;
        }
      } else if (responseJson.contains("success")) {
        // Non-streaming response format (fallback)
        if (responseJson["success"].get<bool>()) {
          if (responseJson.contains("content")) {
            string content = responseJson["content"].get<string>();
            wstring wContent = StringToWstring(content);

            vector<wstring> chunks = SplitIntoChunks(wContent, 100);
            for (const auto &chunk : chunks) {
              Stream(chunk);
              Sleep(50);
            }
          }
        } else if (responseJson.contains("error")) {
          string error = responseJson["error"].get<string>();
          MSGBOX_WARNING(L"Error: " + StringToWstring(error));
        }
        streamComplete = true;
      }

    } catch (json::exception &e) {
      MSGBOX_ERROR(L"Json parse error during streaming: " +
                   StringToWstring(e.what()));
      DEBUG_LOG("JSON parse error during streaming: %s", e.what());
      // Continue receiving - might be partial data
    }
  }

  SetHistoryChat();
}

vector<string> MCPClient::getFilePath() {
  vector<string> currentPath;
  UniquePathSet outPaths;

  // prepare filters for the dialog
  nfdfilteritem_t filterItem[3] = {
      {"Documents", "pdf, doc, docx, xls, xlsx, ppt, pptx"},
      {"Sources code", "c,cpp,cc,js,py,rb,java,kt,swift,go,rs,ts"},
      {"Images", "png,jpg,jpeg,gif,webp"}};

  // show the dialog
  nfdresult_t result = OpenDialogMultiple(outPaths, filterItem, 3);
  if (result == NFD_OKAY) {

    nfdpathsetsize_t numPaths;
    PathSet::Count(outPaths, numPaths);

    nfdpathsetsize_t i;
    for (i = 0; i < numPaths; ++i) {
      UniquePathSetPath path;
      PathSet::GetPath(outPaths, i, path);
      currentPath.push_back(path.get());
    }
    return currentPath;
  } else if (result == NFD_CANCEL) {
  } else {
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

string MCPClient::WstringToString(const wstring &str) {
  if (str.empty())
    return "";

  int sizeNeeded = WideCharToMultiByte(CP_UTF8, // output UTF-8
                                       0, str.c_str(), (int)str.size(), nullptr,
                                       0, nullptr, nullptr);

  string result(sizeNeeded, 0);

  WideCharToMultiByte(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0],
                      sizeNeeded, nullptr, nullptr);

  return result;
}

void MCPClient::SetHistoryChat() {
  wstring messageLoaded;
  if (!isConnected) {
    MSGBOX_ERROR(L"Not connected to WebSocket");
    return;
  }

  // Clear existing history
  historyChat.clear();

  string response =
      SendMessageToWebsocket("{\"id\":\"1\",\"type\":\"history\"}");

  // Save to file for debugging
  ofstream his("history.json");
  if (his.is_open()) {
    his << response << std::endl;
    his.close();
  }

  // Simple JSON parsing for our specific structure
  // Expected format: {"type":"history","status":"ok","data":[{...},{...}]}

  // Find "data":[
  size_t dataStart = response.find("\"data\":[");
  if (dataStart == string::npos) {
    DEBUG_LOG("No data array found in response");
    return;
  }
  dataStart += 8; // Skip past "data":[

  // Find each object in the array
  size_t pos = dataStart;
  while (pos < response.length()) {
    // Find start of object
    size_t objStart = response.find('{', pos);
    if (objStart == string::npos || objStart >= response.length())
      break;

    // Find end of object (matching brace)
    int braceCount = 1;
    size_t objEnd = objStart + 1;
    while (objEnd < response.length() && braceCount > 0) {
      if (response[objEnd] == '{')
        braceCount++;
      else if (response[objEnd] == '}')
        braceCount--;
      objEnd++;
    }

    if (braceCount != 0)
      break;

    string obj = response.substr(objStart, objEnd - objStart);

    // Extract fields from object
    struct historyChat entry;

    // Extract message
    size_t msgStart = obj.find("\"message\":\"");
    if (msgStart != string::npos) {
      msgStart += 11;
      size_t msgEnd = msgStart;
      while (msgEnd < obj.length()) {
        if (obj[msgEnd] == '"' && (msgEnd == 0 || obj[msgEnd - 1] != '\\'))
          break;
        msgEnd++;
      }
      entry.message = obj.substr(msgStart, msgEnd - msgStart);
      messageLoaded = StringToWstring(entry.message) + L"\n";
    }

    // Extract timestamp
    size_t tsStart = obj.find("\"timestamp\":\"");
    if (tsStart != string::npos) {
      tsStart += 13;
      size_t tsEnd = obj.find('"', tsStart);
      if (tsEnd != string::npos) {
        entry.timestamp = obj.substr(tsStart, tsEnd - tsStart);
      }
    }

    // Extract role
    size_t roleStart = obj.find("\"role\":\"");
    if (roleStart != string::npos) {
      roleStart += 8;
      size_t roleEnd = obj.find('"', roleStart);
      if (roleEnd != string::npos) {
        entry.role = obj.substr(roleStart, roleEnd - roleStart);
      }
    }

    historyChat.push_back(entry);

    // Move to next object
    pos = objEnd;

    // Check if we've reached the end of the array
    size_t nextComma = response.find(',', pos);
    size_t arrayEnd = response.find(']', pos);
    if (arrayEnd != string::npos &&
        (nextComma == string::npos || arrayEnd < nextComma)) {
      break;
    }
  }

  // MSGBOX_INFO(L"Loaded " + to_wstring(historyChat.size()) +
  //             L" history entries");
}
} // namespace MCPHelper