// Connect.cpp : Implementation of CConnect

#include "Connect.h"
#include "client/client.hpp"
#include "framework.h"
#include <string>
#include <vector>
#include <winuser.h>

MCPHelper::MCPClient client;
// IDTExtensibility2 Implementation
// Called when the add-in is loaded into Word
STDMETHODIMP
CConnect::OnConnection(IDispatch *Application,
                       AddInDesignerObjects::ext_ConnectMode ConnectMode,
                       IDispatch *AddInInst, SAFEARRAY **custom) {
  UNREFERENCED_PARAMETER(ConnectMode);
  UNREFERENCED_PARAMETER(AddInInst);
  UNREFERENCED_PARAMETER(custom);

  m_pApplication = Application;
  if (m_pApplication) {
    m_pApplication->AddRef();
  }

  // DatabaseCore dbHelper;
  // dbHelper.dbHelper->InsertToRows(
  //     "history_chat",
  //     {{"session_id", "test"}, {"role", "user"}, {"content", "Hello!"}});
  // wstring databaseInfo = dbHelper.dbHelper->ShowTablesWString();
  // wstring rowsInfo = dbHelper.dbHelper->ShowRowsWString("history_chat");
  // Show a message box to confirm the add-in loaded (for debugging)
  MessageBoxW(NULL,
              L"Agentic AI Add-in loaded successfully!\n This Project Created "
              L"By Hylmi on https://hylmithecoder/AI_Agentic_In_Word",
              L"Add-in Info", MB_OK | MB_ICONINFORMATION);

  vector<string> filePathNames = client.getFilePath();

  wstring message = L"Selected files:\n\n";

  for (const auto &path : filePathNames) {
    message += client.StringToWstring(path);
    message += L"\n";
  }

  MessageBoxW(NULL, message.c_str(), L"File Picker Result",
              MB_OK | MB_ICONINFORMATION);

  // MessageBoxW(NULL, databaseInfo.c_str(), L"Database Info",
  //             MB_OK | MB_ICONINFORMATION);

  // MessageBoxW(NULL, rowsInfo.c_str(), L"Rows Info", MB_OK |
  // MB_ICONINFORMATION);

  return S_OK;
}

// Called when the add-in is unloaded
STDMETHODIMP
CConnect::OnDisconnection(AddInDesignerObjects::ext_DisconnectMode RemoveMode,
                          SAFEARRAY **custom) {
  UNREFERENCED_PARAMETER(RemoveMode);
  UNREFERENCED_PARAMETER(custom);

  if (m_pCustomTaskPane) {
    m_pCustomTaskPane->Release();
    m_pCustomTaskPane = nullptr;
  }

  if (m_pApplication) {
    m_pApplication->Release();
    m_pApplication = nullptr;
  }

  MessageBoxW(NULL, L"Agentic AI Add-in unloaded successfully!", L"Add-in Info",
              MB_OK | MB_ICONINFORMATION);

  return S_OK;
}

STDMETHODIMP CConnect::OnAddInsUpdate(SAFEARRAY **custom) {
  UNREFERENCED_PARAMETER(custom);
  return S_OK;
}

STDMETHODIMP CConnect::OnStartupComplete(SAFEARRAY **custom) {
  UNREFERENCED_PARAMETER(custom);
  return S_OK;
}

STDMETHODIMP CConnect::OnBeginShutdown(SAFEARRAY **custom) {
  UNREFERENCED_PARAMETER(custom);
  return S_OK;
}

// ICustomTaskPaneConsumer Implementation
// Called by Office when task pane infrastructure is ready
STDMETHODIMP
CConnect::CTPFactoryAvailable(Office::ICTPFactory *CTPFactoryInst) {
  if (!CTPFactoryInst)
    return E_POINTER;

  HRESULT hr = S_OK;

  // Create the custom task pane using our TaskPaneControl
  // The ProgID must match the registration of our control
  CComBSTR bstrProgId(L"AgenticAIOnWord.TaskPaneControl");
  CComBSTR bstrTitle(L"Agentic AI Assistant");

  // Create a VARIANT for the optional parameter (missing)
  VARIANT varMissing;
  VariantInit(&varMissing);
  varMissing.vt = VT_ERROR;
  varMissing.scode = DISP_E_PARAMNOTFOUND;

  hr = CTPFactoryInst->CreateCTP(bstrProgId, bstrTitle, varMissing,
                                 &m_pCustomTaskPane);

  if (SUCCEEDED(hr) && m_pCustomTaskPane) {
    // Make the task pane visible
    m_pCustomTaskPane->put_Visible(VARIANT_TRUE);

    // Set preferred width
    m_pCustomTaskPane->put_Width(300);
  }

  return hr;
}