// TaskPaneControl.cpp : Implementation of CTaskPaneControl

#include "TaskPaneControl.h"
#include "framework.h"
#include <string>

// Drawing handler - paints the control background
HRESULT CTaskPaneControl::OnDraw(ATL_DRAWINFO &di) {
  RECT &rc = *(RECT *)di.prcBounds;

  // Create a gradient background
  HDC hdc = di.hdcDraw;

  // Fill background with a modern color
  HBRUSH hBrush = CreateSolidBrush(RGB(245, 245, 250));
  FillRect(hdc, &rc, hBrush);
  DeleteObject(hBrush);

  // Draw a subtle top border
  HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 120, 215));
  HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
  MoveToEx(hdc, rc.left, rc.top, nullptr);
  LineTo(hdc, rc.right, rc.top);
  SelectObject(hdc, hOldPen);
  DeleteObject(hPen);

  return S_OK;
}

// Create child controls when window is created
LRESULT CTaskPaneControl::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                   BOOL &bHandled) {
  UNREFERENCED_PARAMETER(uMsg);
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);
  bHandled = TRUE;

  // Create background brush for child controls
  m_hBkBrush = CreateSolidBrush(RGB(245, 245, 250));

  RECT rc;
  GetClientRect(&rc);
  int width = rc.right - rc.left - 20; // 10px padding on each side

  // Create title label
  m_wndTitleLabel.Create(L"STATIC", m_hWnd, CRect(10, 20, 10 + width, 50),
                         L"Agentic AI Assistant",
                         WS_CHILD | WS_VISIBLE | SS_CENTER);

  // Set font for title
  HFONT hTitleFont =
      CreateFont(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  m_wndTitleLabel.SendMessage(WM_SETFONT, (WPARAM)hTitleFont, TRUE);

  std::wstring infoText = L"Agentic AI Chat";

  // Create info label
  m_wndInfoLabel.Create(L"STATIC", m_hWnd, CRect(10, 70, 10 + width, 350),
                        infoText.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT);

  m_wndChatArea.Create(L"STATIC", m_hWnd, CRect(10, 70, 10 + width, 280),
                       L"🤖 AI: Hello Hylmi.\r\nReady to assist you.",
                       WS_CHILD | WS_VISIBLE | SS_LEFT | SS_EDITCONTROL);

  HFONT hChatFont =
      CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
  m_wndChatArea.SendMessage(WM_SETFONT, (WPARAM)hChatFont, TRUE);

  HFONT hInfoFont =
      CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
  m_wndInfoLabel.SendMessage(WM_SETFONT, (WPARAM)hInfoFont, TRUE);

  m_wndSendButton.Create(L"BUTTON", m_hWnd,
                         CRect(10 + width - 60, 290, 10 + width, 325), L"Send",
                         WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);
  m_wndSendButton.SendMessage(WM_SETFONT, (WPARAM)hChatFont, TRUE);

  // Create action button
  // m_wndActionButton.Create(L"BUTTON", m_hWnd, CRect(10, 360, 10 + width,
  // 410),
  //                          L"Get Started",
  //                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON);

  // HFONT hBtnFont =
  //     CreateFont(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
  //     DEFAULT_CHARSET,
  //                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
  //                DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
  // m_wndActionButton.SendMessage(WM_SETFONT, (WPARAM)hBtnFont, TRUE);

  return 0;
}

// Resize child controls when window is resized
LRESULT CTaskPaneControl::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                 BOOL &bHandled) {
  UNREFERENCED_PARAMETER(uMsg);
  UNREFERENCED_PARAMETER(wParam);
  bHandled = TRUE;

  int width = LOWORD(lParam) - 20; // 10px padding on each side

  if (m_wndTitleLabel.IsWindow())
    m_wndTitleLabel.MoveWindow(10, 20, width, 30);

  if (m_wndInfoLabel.IsWindow())
    m_wndInfoLabel.MoveWindow(10, 70, width, 80);

  if (m_wndActionButton.IsWindow())
    m_wndActionButton.MoveWindow(10, 170, width, 35);

  return 0;
}

// Handle background color for static controls
LRESULT CTaskPaneControl::OnCtlColorStatic(UINT uMsg, WPARAM wParam,
                                           LPARAM lParam, BOOL &bHandled) {
  UNREFERENCED_PARAMETER(uMsg);
  UNREFERENCED_PARAMETER(lParam);
  bHandled = TRUE;

  HDC hdc = (HDC)wParam;
  SetBkColor(hdc, RGB(245, 245, 250));
  SetTextColor(hdc, RGB(30, 30, 30));

  return (LRESULT)m_hBkBrush;
}
