// Connect.h : Declaration of the CConnect class
// This is the main COM Add-in class that Word loads

#pragma once
#include "framework.h"
#include "resource.h"

// Define CLSIDs/LIBIDs directly (must match AgenticAIOnWord.idl)
// Library GUID: {cddbcd6c-2230-4b77-bd3c-2d09ed326605}
static const GUID LIBID_AgenticAIOnWordLib_Connect = {
    0xcddbcd6c,
    0x2230,
    0x4b77,
    {0xbd, 0x3c, 0x2d, 0x09, 0xed, 0x32, 0x66, 0x05}};

// Connect CLSID: {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
static const GUID CLSID_Connect = {
    0xA1B2C3D4,
    0xE5F6,
    0x7890,
    {0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78, 0x90}};

using namespace ATL;

// CConnect - Main add-in class implementing IDTExtensibility2 and
// ICustomTaskPaneConsumer
// namespace ConnectHandler {
class ATL_NO_VTABLE CConnect
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CConnect, &CLSID_Connect>,
      public IDispatchImpl<AddInDesignerObjects::_IDTExtensibility2,
                           &AddInDesignerObjects::IID__IDTExtensibility2,
                           &AddInDesignerObjects::LIBID_AddInDesignerObjects, 1,
                           0>,
      public IDispatchImpl<Office::ICustomTaskPaneConsumer,
                           &Office::IID_ICustomTaskPaneConsumer,
                           &Office::LIBID_Office, 2, 0> {
public:
  CConnect() : m_pApplication(nullptr), m_pCustomTaskPane(nullptr) {}

  DECLARE_REGISTRY_RESOURCEID(IDR_CONNECT)

  BEGIN_COM_MAP(CConnect)
  COM_INTERFACE_ENTRY(AddInDesignerObjects::_IDTExtensibility2)
  COM_INTERFACE_ENTRY(Office::ICustomTaskPaneConsumer)
  COM_INTERFACE_ENTRY2(IDispatch, AddInDesignerObjects::_IDTExtensibility2)
  END_COM_MAP()

  DECLARE_PROTECT_FINAL_CONSTRUCT()

  HRESULT FinalConstruct() { return S_OK; }
  void FinalRelease() {}

public:
  // IDTExtensibility2 Methods
  STDMETHOD(OnConnection)(IDispatch *Application,
                          AddInDesignerObjects::ext_ConnectMode ConnectMode,
                          IDispatch *AddInInst, SAFEARRAY **custom) override;
  STDMETHOD(OnDisconnection)(
      AddInDesignerObjects::ext_DisconnectMode RemoveMode,
      SAFEARRAY **custom) override;
  STDMETHOD(OnAddInsUpdate)(SAFEARRAY **custom) override;
  STDMETHOD(OnStartupComplete)(SAFEARRAY **custom) override;
  STDMETHOD(OnBeginShutdown)(SAFEARRAY **custom) override;

  // ICustomTaskPaneConsumer Methods
  STDMETHOD(CTPFactoryAvailable)(Office::ICTPFactory *CTPFactoryInst) override;

private:
  IDispatch *m_pApplication;
  Office::_CustomTaskPane *m_pCustomTaskPane;
};

// } // namespace ConnectHandler

OBJECT_ENTRY_AUTO(CLSID_Connect, CConnect)