// TaskPaneControl.h : Declaration of the CTaskPaneControl ActiveX Control
// This control hosts the Task Pane UI content

#pragma once
#include "client/client.hpp"
#include "framework.h"
#include "resource.h"
#include <string>

using namespace std;
using namespace MCPHelper;

// Define CLSIDs/LIBIDs directly (must match AgenticAIOnWord.idl)
// Library GUID: {cddbcd6c-2230-4b77-bd3c-2d09ed326605}
static const GUID LIBID_AgenticAIOnWordLib_TaskPane = {
    0xcddbcd6c,
    0x2230,
    0x4b77,
    {0xbd, 0x3c, 0x2d, 0x09, 0xed, 0x32, 0x66, 0x05}};

// TaskPaneControl CLSID: {B2C3D4E5-F6A7-8901-BCDE-F23456789012}
static const GUID CLSID_TaskPaneControl = {
    0xB2C3D4E5,
    0xF6A7,
    0x8901,
    {0xBC, 0xDE, 0xF2, 0x34, 0x56, 0x78, 0x90, 0x12}};

using namespace ATL;

// CTaskPaneControl - ActiveX control that displays in the Task Pane
class ATL_NO_VTABLE CTaskPaneControl
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CTaskPaneControl, &CLSID_TaskPaneControl>,
      public CComControl<CTaskPaneControl>,
      public IDispatchImpl<IDispatch, &IID_IDispatch,
                           &LIBID_AgenticAIOnWordLib_TaskPane, 1, 0>,
      public IOleControlImpl<CTaskPaneControl>,
      public IOleObjectImpl<CTaskPaneControl>,
      public IOleInPlaceActiveObjectImpl<CTaskPaneControl>,
      public IViewObjectExImpl<CTaskPaneControl>,
      public IOleInPlaceObjectWindowlessImpl<CTaskPaneControl>,
      public IPersistStorageImpl<CTaskPaneControl>,
      public IQuickActivateImpl<CTaskPaneControl>,
      public IDataObjectImpl<CTaskPaneControl>,
      public IProvideClassInfo2Impl<&CLSID_TaskPaneControl, NULL,
                                    &LIBID_AgenticAIOnWordLib_TaskPane>,
      public IObjectSafetyImpl<CTaskPaneControl,
                               INTERFACESAFE_FOR_UNTRUSTED_CALLER> {
public:
  CTaskPaneControl() { m_bWindowOnly = TRUE; }

  DECLARE_REGISTRY_RESOURCEID(IDR_TASKPANECONTROL)

  BEGIN_COM_MAP(CTaskPaneControl)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(IViewObjectEx)
  COM_INTERFACE_ENTRY(IViewObject2)
  COM_INTERFACE_ENTRY(IViewObject)
  COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
  COM_INTERFACE_ENTRY(IOleInPlaceObject)
  COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
  COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
  COM_INTERFACE_ENTRY(IOleControl)
  COM_INTERFACE_ENTRY(IOleObject)
  COM_INTERFACE_ENTRY(IPersistStorage)
  COM_INTERFACE_ENTRY(IQuickActivate)
  COM_INTERFACE_ENTRY(IDataObject)
  COM_INTERFACE_ENTRY(IProvideClassInfo)
  COM_INTERFACE_ENTRY(IProvideClassInfo2)
  COM_INTERFACE_ENTRY(IObjectSafety)
  END_COM_MAP()

  BEGIN_MSG_MAP(CTaskPaneControl)
  CHAIN_MSG_MAP(CComControl<CTaskPaneControl>)
  MESSAGE_HANDLER(WM_CREATE, OnCreate)
  MESSAGE_HANDLER(WM_SIZE, OnSize)
  MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
  MESSAGE_HANDLER(WM_COMMAND, OnCommand)
  DEFAULT_REFLECTION_HANDLER()
  END_MSG_MAP()

  DECLARE_PROTECT_FINAL_CONSTRUCT()

  HRESULT FinalConstruct() { return S_OK; }
  void FinalRelease() {}

  // Drawing
  HRESULT OnDraw(ATL_DRAWINFO &di);

  // IViewObjectEx
  DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

  // Message handlers
  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
  LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
  LRESULT OnCtlColorStatic(UINT uMsg, WPARAM wParam, LPARAM lParam,
                           BOOL &bHandled);
  LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

private:
  MCPClient client;

  // Child controls
  CWindow m_wndTitleLabel;
  CWindow m_wndInfoLabel;
  CWindow m_wndActionButton;
  CWindow m_wndChatArea;
  CWindow m_wndSendButton;
  CWindow m_wndFileButton;
  CWindow m_wndFileLabel;
  CWindow m_wndInputEdit;

  // Background brushes
  HBRUSH m_hBkBrush = nullptr;
  HBRUSH m_hAccentBrush = nullptr;

  // Fonts
  HFONT m_hTitleFont = nullptr;
  HFONT m_hTextFont = nullptr;
  HFONT m_hBtnFont = nullptr;

  // Selected file paths
  vector<string> m_selectedFiles;

  // Helper to update file label text
  void UpdateFileLabel();
};

OBJECT_ENTRY_AUTO(CLSID_TaskPaneControl, CTaskPaneControl)
