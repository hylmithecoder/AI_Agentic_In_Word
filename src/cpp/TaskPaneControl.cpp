// TaskPaneControl.cpp : Implementation of CTaskPaneControl

#include "TaskPaneControl.h"
#include "framework.h"
#include "resource.h"
#include <string>

// Modern color scheme
namespace Colors {
const COLORREF Background = RGB(250, 250, 252);
const COLORREF AccentBlue = RGB(0, 120, 215);
const COLORREF AccentLight = RGB(230, 240, 250);
const COLORREF TextPrimary = RGB(32, 32, 32);
const COLORREF TextSecondary = RGB(100, 100, 100);
const COLORREF BorderLight = RGB(220, 220, 225);
} // namespace Colors

// Drawing handler - paints the control background
HRESULT CTaskPaneControl::OnDraw(ATL_DRAWINFO &di) {
  RECT &rc = *(RECT *)di.prcBounds;
  HDC hdc = di.hdcDraw;

  // Fill background with modern light color
  HBRUSH hBrush = CreateSolidBrush(Colors::Background);
  FillRect(hdc, &rc, hBrush);
  DeleteObject(hBrush);

  // Draw accent header bar at top
  RECT headerRect = rc;
  headerRect.bottom = rc.top + 4;
  HBRUSH hAccentBrush = CreateSolidBrush(Colors::AccentBlue);
  FillRect(hdc, &headerRect, hAccentBrush);
  DeleteObject(hAccentBrush);

  return S_OK;
}

// Create child controls when window is created
LRESULT CTaskPaneControl::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                   BOOL &bHandled) {
  UNREFERENCED_PARAMETER(uMsg);
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);
  bHandled = TRUE;

  // Create brushes
  m_hBkBrush = CreateSolidBrush(Colors::Background);
  m_hAccentBrush = CreateSolidBrush(Colors::AccentLight);

  // Create fonts
  m_hTitleFont =
      CreateFont(22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  m_hTextFont =
      CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  m_hBtnFont =
      CreateFont(13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  RECT rc;
  GetClientRect(&rc);
  int width = rc.right - rc.left - 24; // 12px padding on each side
  int xPos = 12;
  int yPos = 16;

  // ===== Title Section =====
  m_wndTitleLabel.Create(
      L"STATIC", m_hWnd, CRect(xPos, yPos, xPos + width, yPos + 28),
      L"🤖 Agentic AI Assistant", WS_CHILD | WS_VISIBLE | SS_LEFT);
  m_wndTitleLabel.SendMessage(WM_SETFONT, (WPARAM)m_hTitleFont, TRUE);
  yPos += 40;

  // ===== File Selection Section =====
  // File button (📁)
  m_wndFileButton.Create(
      L"BUTTON", m_hWnd, CRect(xPos, yPos, xPos + 120, yPos + 32),
      L"📁 Select Files", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0,
      IDC_FILE_BUTTON);
  m_wndFileButton.SendMessage(WM_SETFONT, (WPARAM)m_hBtnFont, TRUE);

  // File label (shows selected files)
  m_wndFileLabel.Create(
      L"STATIC", m_hWnd, CRect(xPos + 130, yPos, xPos + width, yPos + 32),
      L"No files selected", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_PATHELLIPSIS);
  m_wndFileLabel.SendMessage(WM_SETFONT, (WPARAM)m_hTextFont, TRUE);
  yPos += 44;

  // ===== Chat Area Section =====
  m_wndChatArea.Create(
      L"STATIC", m_hWnd, CRect(xPos, yPos, xPos + width, yPos + 180),
      L"💬 AI: Hello! I'm your Agentic AI assistant.\r\n\r\nI can help you "
      L"with:\r\n• Editing documents\r\n• Answering questions\r\n• Code "
      L"analysis\r\n\r\nSelect a file and ask me anything!",
      WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER);
  m_wndChatArea.SendMessage(WM_SETFONT, (WPARAM)m_hTextFont, TRUE);
  yPos += 192;

  // ===== Input Section =====
  m_wndInputEdit.Create(L"EDIT", m_hWnd,
                        CRect(xPos, yPos, xPos + width - 70, yPos + 60), L"",
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
                            ES_AUTOVSCROLL | ES_WANTRETURN,
                        0, IDC_INPUT_EDIT);
  m_wndInputEdit.SendMessage(WM_SETFONT, (WPARAM)m_hTextFont, TRUE);
  m_wndInputEdit.SendMessage(EM_SETCUEBANNER, TRUE,
                             (LPARAM)L"Type your message...");

  // Send button
  m_wndSendButton.Create(
      L"BUTTON", m_hWnd,
      CRect(xPos + width - 60, yPos, xPos + width, yPos + 60), L"Send ➤",
      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, IDC_SEND_BUTTON);
  m_wndSendButton.SendMessage(WM_SETFONT, (WPARAM)m_hBtnFont, TRUE);
  yPos += 72;

  // ===== Info Label =====
  m_wndInfoLabel.Create(
      L"STATIC", m_hWnd, CRect(xPos, yPos, xPos + width, yPos + 20),
      L"Powered by Agentic AI • v1.0", WS_CHILD | WS_VISIBLE | SS_CENTER);
  m_wndInfoLabel.SendMessage(WM_SETFONT, (WPARAM)m_hTextFont, TRUE);

  return 0;
}

// Resize child controls when window is resized
LRESULT CTaskPaneControl::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                 BOOL &bHandled) {
  UNREFERENCED_PARAMETER(uMsg);
  UNREFERENCED_PARAMETER(wParam);
  bHandled = TRUE;

  int clientWidth = LOWORD(lParam);
  int clientHeight = HIWORD(lParam);
  int width = clientWidth - 24; // 12px padding on each side
  int xPos = 12;
  int yPos = 16;

  if (m_wndTitleLabel.IsWindow()) {
    m_wndTitleLabel.MoveWindow(xPos, yPos, width, 28);
    yPos += 40;
  }

  if (m_wndFileButton.IsWindow()) {
    m_wndFileButton.MoveWindow(xPos, yPos, 120, 32);
  }
  if (m_wndFileLabel.IsWindow()) {
    m_wndFileLabel.MoveWindow(xPos + 130, yPos, width - 130, 32);
  }
  yPos += 44;

  // Calculate remaining height for chat area
  int chatHeight = clientHeight - yPos - 110; // Reserve space for input + info
  if (chatHeight < 80)
    chatHeight = 80;

  if (m_wndChatArea.IsWindow()) {
    m_wndChatArea.MoveWindow(xPos, yPos, width, chatHeight);
    yPos += chatHeight + 12;
  }

  if (m_wndInputEdit.IsWindow()) {
    m_wndInputEdit.MoveWindow(xPos, yPos, width - 70, 60);
  }
  if (m_wndSendButton.IsWindow()) {
    m_wndSendButton.MoveWindow(xPos + width - 60, yPos, 60, 60);
  }
  yPos += 72;

  if (m_wndInfoLabel.IsWindow()) {
    m_wndInfoLabel.MoveWindow(xPos, yPos, width, 20);
  }

  return 0;
}

// Handle background color for static controls
LRESULT CTaskPaneControl::OnCtlColorStatic(UINT uMsg, WPARAM wParam,
                                           LPARAM lParam, BOOL &bHandled) {
  UNREFERENCED_PARAMETER(uMsg);
  bHandled = TRUE;

  HDC hdc = (HDC)wParam;
  HWND hCtl = (HWND)lParam;

  SetBkColor(hdc, Colors::Background);

  // Title gets primary color
  if (hCtl == m_wndTitleLabel.m_hWnd) {
    SetTextColor(hdc, Colors::AccentBlue);
  }
  // Info label gets secondary color
  else if (hCtl == m_wndInfoLabel.m_hWnd || hCtl == m_wndFileLabel.m_hWnd) {
    SetTextColor(hdc, Colors::TextSecondary);
  }
  // Default text color
  else {
    SetTextColor(hdc, Colors::TextPrimary);
  }

  return (LRESULT)m_hBkBrush;
}

// Handle button commands
LRESULT CTaskPaneControl::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                    BOOL &bHandled) {
  UNREFERENCED_PARAMETER(uMsg);
  UNREFERENCED_PARAMETER(lParam);

  WORD ctrlId = LOWORD(wParam);
  WORD notifyCode = HIWORD(wParam);

  if (notifyCode == BN_CLICKED) {
    if (ctrlId == IDC_FILE_BUTTON) {
      // Open file dialog using MCPClient
      m_selectedFiles = client.getFilePath();
      UpdateFileLabel();
      bHandled = TRUE;
      return 0;
    } else if (ctrlId == IDC_SEND_BUTTON) {
      // TODO: Handle send button click
      // Get text from input edit
      int len = m_wndInputEdit.GetWindowTextLength();
      if (len > 0) {
        wchar_t *buffer = new wchar_t[len + 1];
        m_wndInputEdit.GetWindowText(buffer, len + 1);

        // For now, show a message box with the input
        MessageBoxW(m_hWnd, buffer, L"Message Sent",
                    MB_OK | MB_ICONINFORMATION);

        // Clear input
        m_wndInputEdit.SetWindowText(L"");
        delete[] buffer;
      }
      bHandled = TRUE;
      return 0;
    }
  }

  bHandled = FALSE;
  return 0;
}

// Update file label with selected file paths
void CTaskPaneControl::UpdateFileLabel() {
  if (m_selectedFiles.empty()) {
    m_wndFileLabel.SetWindowText(L"No files selected");
    return;
  }

  std::wstring labelText;
  if (m_selectedFiles.size() == 1) {
    // Show the single file name
    std::string &path = m_selectedFiles[0];
    size_t lastSlash = path.find_last_of("\\/");
    std::string fileName =
        (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
    labelText = std::wstring(fileName.begin(), fileName.end());
  } else {
    // Show count of files
    labelText = std::to_wstring(m_selectedFiles.size()) + L" files selected";
  }

  m_wndFileLabel.SetWindowText(labelText.c_str());
}
