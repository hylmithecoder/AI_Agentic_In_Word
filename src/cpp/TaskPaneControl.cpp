// TaskPaneControl.cpp : Implementation of CTaskPaneControl

#include "TaskPaneControl.h"
#include "framework.h"
#include "resource.h"
#include <string>
#include <wingdi.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// Modern D2D color scheme
namespace D2DColors {
const D2D1_COLOR_F Background = {0.98f, 0.98f, 0.99f, 1.0f};
const D2D1_COLOR_F AccentBlue = {0.0f, 0.47f, 0.84f, 1.0f};
const D2D1_COLOR_F AccentLight = {0.90f, 0.94f, 0.98f, 1.0f};
const D2D1_COLOR_F TextPrimary = {0.125f, 0.125f, 0.125f, 1.0f};
const D2D1_COLOR_F TextSecondary = {0.4f, 0.4f, 0.4f, 1.0f};
const D2D1_COLOR_F BorderLight = {0.86f, 0.86f, 0.88f, 1.0f};
const D2D1_COLOR_F White = {1.0f, 1.0f, 1.0f, 1.0f};
} // namespace D2DColors

// GDI color scheme for child controls
namespace Colors {
const COLORREF Background = RGB(250, 250, 252);
const COLORREF AccentBlue = RGB(0, 120, 215);
const COLORREF AccentLight = RGB(230, 240, 250);
const COLORREF TextPrimary = RGB(32, 32, 32);
const COLORREF TextSecondary = RGB(100, 100, 100);
const COLORREF BorderLight = RGB(220, 220, 225);
} // namespace Colors

// Initialize Direct2D and DirectWrite resources
HRESULT CTaskPaneControl::InitD2DResources() {
  HRESULT hr = S_OK;

  // Create D2D Factory
  if (!m_d2dFactory) {
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2dFactory);
    if (FAILED(hr))
      return hr;
  }

  // Create DirectWrite Factory
  if (!m_dwriteFactory) {
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                             __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown **>(&m_dwriteFactory));
    if (FAILED(hr))
      return hr;
  }

  // Create Title Text Format (Segoe UI Emoji for emoji support)
  if (!m_titleTextFormat) {
    hr = m_dwriteFactory->CreateTextFormat(
        L"Segoe UI Emoji", // Font family (supports emojis)
        nullptr,           // Font collection
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        20.0f, // Font size
        L"",   // Locale
        &m_titleTextFormat);
    if (FAILED(hr))
      return hr;

    m_titleTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_titleTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
  }

  // Create Body Text Format
  if (!m_bodyTextFormat) {
    hr = m_dwriteFactory->CreateTextFormat(
        L"Segoe UI Emoji", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"",
        &m_bodyTextFormat);
    if (FAILED(hr))
      return hr;

    m_bodyTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_bodyTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    m_bodyTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
  }

  return hr;
}

// Create or recreate render target for the window
HRESULT CTaskPaneControl::CreateRenderTarget() {
  if (!m_d2dFactory)
    return E_FAIL;

  // Release existing render target
  m_renderTarget.Release();
  m_textBrush.Release();
  m_accentBrush.Release();
  m_bgBrush.Release();

  RECT rc;
  GetClientRect(&rc);

  D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

  HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(),
      D2D1::HwndRenderTargetProperties(m_hWnd, size), &m_renderTarget);

  if (SUCCEEDED(hr)) {
    m_renderTarget->CreateSolidColorBrush(D2DColors::TextPrimary, &m_textBrush);
    m_renderTarget->CreateSolidColorBrush(D2DColors::AccentBlue,
                                          &m_accentBrush);
    m_renderTarget->CreateSolidColorBrush(D2DColors::Background, &m_bgBrush);
  }

  return hr;
}

// Drawing handler - uses Direct2D for emoji-capable text rendering
HRESULT CTaskPaneControl::OnDraw(ATL_DRAWINFO &di) {
  RECT &rc = *(RECT *)di.prcBounds;

  // Ensure D2D resources are created
  if (!m_renderTarget) {
    if (FAILED(CreateRenderTarget()))
      return S_OK; // Fallback gracefully
  }

  // Check if render target is still valid
  D2D1_SIZE_U targetSize = m_renderTarget->GetPixelSize();
  UINT width = rc.right - rc.left;
  UINT height = rc.bottom - rc.top;
  if (targetSize.width != width || targetSize.height != height) {
    m_renderTarget->Resize(D2D1::SizeU(width, height));
  }

  m_renderTarget->BeginDraw();

  // Clear background
  m_renderTarget->Clear(D2DColors::Background);

  // Draw accent header bar at top
  D2D1_RECT_F headerRect = D2D1::RectF(0.0f, 0.0f, (FLOAT)width, 4.0f);
  m_renderTarget->FillRectangle(headerRect, m_accentBrush);

  // Draw title with emoji support 🤖
  const wchar_t *title = L"\xD83E\xDD16 Agentic AI Assistant";
  D2D1_RECT_F titleRect = D2D1::RectF(12.0f, 16.0f, (FLOAT)(width - 12), 48.0f);
  m_renderTarget->DrawText(title, (UINT32)wcslen(title), m_titleTextFormat,
                           titleRect, m_accentBrush);

  HRESULT hr = m_renderTarget->EndDraw();

  // If the render target was lost, recreate it
  if (hr == D2DERR_RECREATE_TARGET) {
    CreateRenderTarget();
  }

  return S_OK;
}

// Create child controls when window is created
LRESULT CTaskPaneControl::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                   BOOL &bHandled) {
  UNREFERENCED_PARAMETER(uMsg);
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);
  bHandled = TRUE;

  // Initialize Direct2D/DirectWrite
  InitD2DResources();

  // Create GDI brushes for child controls
  m_hBkBrush = CreateSolidBrush(Colors::Background);
  m_hAccentBrush = CreateSolidBrush(Colors::AccentLight);

  // Create fonts for child controls (Segoe UI Emoji for emoji support)
  m_hTitleFont =
      CreateFont(22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 VARIABLE_PITCH, L"Segoe UI Emoji");

  m_hTextFont =
      CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 VARIABLE_PITCH, L"Segoe UI Emoji");

  m_hBtnFont =
      CreateFont(13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 VARIABLE_PITCH, L"Segoe UI");

  RECT rc;
  GetClientRect(&rc);
  int width = rc.right - rc.left - 24; // 12px padding on each side
  int xPos = 12;
  int yPos = 56; // Start after D2D-rendered title

  // ===== File Selection Section =====
  // File button (📁)
  const wchar_t *fileButtonLabel = L"\xD83D\xDCC1 Select Files";
  m_wndFileButton.Create(L"BUTTON", m_hWnd,
                         CRect(xPos, yPos, xPos + 130, yPos + 32),
                         fileButtonLabel, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                         0, IDC_FILE_BUTTON);
  m_wndFileButton.SendMessage(WM_SETFONT, (WPARAM)m_hTextFont, TRUE);

  // File label (shows selected files)
  m_wndFileLabel.Create(
      L"STATIC", m_hWnd, CRect(xPos + 140, yPos, xPos + width, yPos + 32),
      L"No files selected", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_PATHELLIPSIS);
  m_wndFileLabel.SendMessage(WM_SETFONT, (WPARAM)m_hTextFont, TRUE);
  yPos += 44;

  const wchar_t *helloMessage = L"\xD83D\xDCAC AI: Hello! I'm your Agentic AI "
                                L"assistant.\r\n\r\nI can help you "
                                L"with:\r\n  \x2022 Editing documents\r\n  "
                                L"\x2022 Answering questions\r\n  \x2022 Code "
                                L"analysis\r\n\r\nSelect a file and ask me "
                                L"anything!";
  // ===== Chat Area Section =====
  m_wndChatArea.Create(
      L"STATIC", m_hWnd, CRect(xPos, yPos, xPos + width, yPos + 180),
      helloMessage, WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER);
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

  const wchar_t *labelButton = L"Send \x27A4";
  // Send button
  m_wndSendButton.Create(
      L"BUTTON", m_hWnd,
      CRect(xPos + width - 60, yPos, xPos + width, yPos + 60), labelButton,
      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, IDC_SEND_BUTTON);
  m_wndSendButton.SendMessage(WM_SETFONT, (WPARAM)m_hBtnFont, TRUE);
  yPos += 72;

  const wchar_t *infoLabel = L"Powered by Agentic AI \x2022 v1.0";
  // ===== Info Label =====
  m_wndInfoLabel.Create(L"STATIC", m_hWnd,
                        CRect(xPos, yPos, xPos + width, yPos + 20), infoLabel,
                        WS_CHILD | WS_VISIBLE | SS_CENTER);
  m_wndInfoLabel.SendMessage(WM_SETFONT, (WPARAM)m_hTextFont, TRUE);

  return 0;
}

// Resize child controls and render target when window is resized
LRESULT CTaskPaneControl::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                 BOOL &bHandled) {
  UNREFERENCED_PARAMETER(uMsg);
  UNREFERENCED_PARAMETER(wParam);
  bHandled = TRUE;

  int clientWidth = LOWORD(lParam);
  int clientHeight = HIWORD(lParam);
  int width = clientWidth - 24; // 12px padding on each side
  int xPos = 12;
  int yPos = 56; // After D2D-rendered title

  // Resize render target if it exists
  if (m_renderTarget) {
    m_renderTarget->Resize(D2D1::SizeU(clientWidth, clientHeight));
  }

  if (m_wndFileButton.IsWindow()) {
    m_wndFileButton.MoveWindow(xPos, yPos, 130, 32);
  }
  if (m_wndFileLabel.IsWindow()) {
    m_wndFileLabel.MoveWindow(xPos + 140, yPos, width - 140, 32);
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

  // Invalidate to trigger redraw
  InvalidateRect(NULL, TRUE);

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

  // Info label and file label get secondary color
  if (hCtl == m_wndInfoLabel.m_hWnd || hCtl == m_wndFileLabel.m_hWnd) {
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
      // Handle send button click
      int len = m_wndInputEdit.GetWindowTextLength();
      if (len > 0) {
        wchar_t *buffer = new wchar_t[len + 1];
        m_wndInputEdit.GetWindowText(buffer, len + 1);

        // For now, show a message box with the input
        ::MSGBOX_INFO(L"Agentic Extension", buffer);

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
