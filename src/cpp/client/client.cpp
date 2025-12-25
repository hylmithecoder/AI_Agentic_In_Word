#include "client/client.hpp"

namespace MCPHelper {

bool MCPClient::ConnectToMCP() {
  isConnected = true;
  return true;
}

vector<string> MCPClient::getFilePath() {
  vector<string> currentPath;
  NFD::UniquePathSet outPaths;

  // prepare filters for the dialog
  nfdfilteritem_t filterItem[2] = {{"Source code", "c,cpp,cc"},
                                   {"Headers", "h,hpp"}};

  // show the dialog
  nfdresult_t result = NFD::OpenDialogMultiple(outPaths, filterItem, 2);
  if (result == NFD_OKAY) {
    cout << "Success!" << endl;

    nfdpathsetsize_t numPaths;
    NFD::PathSet::Count(outPaths, numPaths);

    nfdpathsetsize_t i;
    for (i = 0; i < numPaths; ++i) {
      NFD::UniquePathSetPath path;
      NFD::PathSet::GetPath(outPaths, i, path);
      currentPath.push_back(path.get());
      cout << "Path " << i << ": " << path.get() << endl;
    }
    return currentPath;
  } else if (result == NFD_CANCEL) {
    cout << "User pressed cancel." << endl;
  } else {
    cout << "Error: " << NFD::GetError() << endl;
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
} // namespace MCPHelper