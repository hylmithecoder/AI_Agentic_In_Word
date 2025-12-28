#include "client/client.hpp"
#include "debugger.hpp"
#include <oleauto.h>

namespace MCPHelper {

// Write response to end of current Word document
void MCPClient::WriteNonStream(const wstring &response) {
  if (!s_pWordApp) {
    MSGBOX_ERROR(L"No Word Application available");
    return;
  }

  // Get ActiveDocument property from Word Application
  DISPID dispid;
  OLECHAR *szMember = (OLECHAR *)L"ActiveDocument";
  HRESULT hr = s_pWordApp->GetIDsOfNames(IID_NULL, &szMember, 1,
                                         LOCALE_USER_DEFAULT, &dispid);

  if (FAILED(hr)) {
    MSGBOX_ERROR(L"No active document to write to");
    return;
  }

  DISPPARAMS dp = {NULL, NULL, 0, 0};
  VARIANT docResult;
  VariantInit(&docResult);

  hr = s_pWordApp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT,
                          DISPATCH_PROPERTYGET, &dp, &docResult, NULL, NULL);

  if (FAILED(hr) || docResult.vt != VT_DISPATCH || !docResult.pdispVal) {
    VariantClear(&docResult);
    MSGBOX_ERROR(L"Failed to get active document");
    return;
  }

  IDispatch *pDoc = docResult.pdispVal;

  // Get Content property (returns Range)
  szMember = (OLECHAR *)L"Content";
  hr =
      pDoc->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT, &dispid);

  if (FAILED(hr)) {
    pDoc->Release();
    MSGBOX_ERROR(L"Failed to get Content property");
    return;
  }

  VARIANT contentResult;
  VariantInit(&contentResult);
  hr = pDoc->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
                    &dp, &contentResult, NULL, NULL);

  if (FAILED(hr) || contentResult.vt != VT_DISPATCH ||
      !contentResult.pdispVal) {
    VariantClear(&contentResult);
    pDoc->Release();
    MSGBOX_ERROR(L"Failed to get document content range");
    return;
  }

  IDispatch *pRange = contentResult.pdispVal;

  // Call InsertAfter method on the Range to insert text at end
  szMember = (OLECHAR *)L"InsertAfter";
  hr = pRange->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT,
                             &dispid);

  if (FAILED(hr)) {
    pRange->Release();
    pDoc->Release();
    MSGBOX_ERROR(L"Failed to get InsertAfter method");
    return;
  }

  // Prepare the text to insert with newlines before
  wstring textToInsert = L"\n\n--- AI Response ---\n" + response + L"\n";

  // Create BSTR from wstring
  BSTR bstrText = SysAllocString(textToInsert.c_str());
  if (!bstrText) {
    pRange->Release();
    pDoc->Release();
    MSGBOX_ERROR(L"Failed to allocate string");
    return;
  }

  VARIANT textArg;
  VariantInit(&textArg);
  textArg.vt = VT_BSTR;
  textArg.bstrVal = bstrText;

  // Set up DISPPARAMS for InsertAfter call
  DISPPARAMS insertParams;
  insertParams.rgvarg = &textArg;
  insertParams.rgdispidNamedArgs = NULL;
  insertParams.cArgs = 1;
  insertParams.cNamedArgs = 0;

  VARIANT vResult;
  VariantInit(&vResult);

  hr = pRange->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
                      &insertParams, &vResult, NULL, NULL);

  SysFreeString(bstrText);
  VariantClear(&vResult);
  pRange->Release();
  pDoc->Release();

  if (FAILED(hr)) {
    MSGBOX_ERROR(L"Failed to insert text into document");
    return;
  }

  MSGBOX_INFO(L"Successfully wrote response to Word document");
}

} // namespace MCPHelper