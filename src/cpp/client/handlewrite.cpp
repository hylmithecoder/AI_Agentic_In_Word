#include "client/client.hpp"

namespace MCPHelper {

// Write response to end of current Word document
void MCPClient::Write(const wstring &response) {
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

  // Create BSTR from wstring
  BSTR bstrText = SysAllocString(response.c_str());
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
}

// Initialize streaming context (call once before streaming)
MCPClient::StreamContext MCPClient::InitStreamContext() {
  StreamContext ctx;

  if (!s_pWordApp) {
    return ctx;
  }

  // Get ActiveDocument
  DISPID dispid;
  OLECHAR *szMember = (OLECHAR *)L"ActiveDocument";
  HRESULT hr = s_pWordApp->GetIDsOfNames(IID_NULL, &szMember, 1,
                                         LOCALE_USER_DEFAULT, &dispid);
  if (FAILED(hr))
    return ctx;

  DISPPARAMS dp = {NULL, NULL, 0, 0};
  VARIANT docResult;
  VariantInit(&docResult);

  hr = s_pWordApp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT,
                          DISPATCH_PROPERTYGET, &dp, &docResult, NULL, NULL);
  if (FAILED(hr))
    return ctx;

  ctx.pDoc = docResult.pdispVal;

  // Get Content Range
  szMember = (OLECHAR *)L"Content";
  hr = ctx.pDoc->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT,
                               &dispid);
  if (FAILED(hr)) {
    ctx.pDoc->Release();
    return ctx;
  }

  VARIANT contentResult;
  VariantInit(&contentResult);
  hr = ctx.pDoc->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT,
                        DISPATCH_PROPERTYGET, &dp, &contentResult, NULL, NULL);
  if (FAILED(hr)) {
    ctx.pDoc->Release();
    return ctx;
  }

  ctx.pRange = contentResult.pdispVal;

  // Cache InsertAfter DISPID
  szMember = (OLECHAR *)L"InsertAfter";
  hr = ctx.pRange->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT,
                                 &ctx.insertAfterDispId);
  if (FAILED(hr)) {
    ctx.pRange->Release();
    ctx.pDoc->Release();
    return ctx;
  }

  ctx.isValid = true;
  return ctx;
}

// Write using cached context (much faster)
void MCPClient::WriteWithContext(StreamContext &ctx, const wstring &text) {
  if (!ctx.isValid)
    return;

  BSTR bstrText = SysAllocString(text.c_str());
  if (!bstrText)
    return;

  VARIANT textArg;
  VariantInit(&textArg);
  textArg.vt = VT_BSTR;
  textArg.bstrVal = bstrText;

  DISPPARAMS insertParams;
  insertParams.rgvarg = &textArg;
  insertParams.rgdispidNamedArgs = NULL;
  insertParams.cArgs = 1;
  insertParams.cNamedArgs = 0;

  VARIANT vResult;
  VariantInit(&vResult);

  ctx.pRange->Invoke(ctx.insertAfterDispId, IID_NULL, LOCALE_USER_DEFAULT,
                     DISPATCH_METHOD, &insertParams, &vResult, NULL, NULL);

  SysFreeString(bstrText);
  VariantClear(&vResult);
}

// Cleanup context
void MCPClient::CleanupStreamContext(StreamContext &ctx) {
  if (ctx.pRange)
    ctx.pRange->Release();
  if (ctx.pDoc)
    ctx.pDoc->Release();
  ctx.isValid = false;
}

// Optimized Stream using context (FASTEST)
void MCPClient::Stream(const wstring &message) {
  if (message.empty())
    return;

  StreamContext ctx = InitStreamContext();
  if (!ctx.isValid) {
    MSGBOX_ERROR(L"Failed to initialize stream context");
    return;
  }

  const size_t CHUNK_SIZE = 50;
  const DWORD DELAY_MS = 50;

  wstring buffer;
  buffer.reserve(CHUNK_SIZE);

  for (size_t i = 0; i < message.length(); i++) {
    buffer += message[i];

    if (buffer.length() >= CHUNK_SIZE || i == message.length() - 1) {
      WriteWithContext(ctx, buffer);
      buffer.clear();

      if (i < message.length() - 1) {
        Sleep(DELAY_MS);
      }
    }
  }

  CleanupStreamContext(ctx);
}

// Alternative: Stream by words (more natural)
void MCPClient::StreamByWords(const wstring &message) {
  if (!s_pWordApp) {
    MSGBOX_ERROR(L"No Word Application available");
    return;
  }

  if (message.empty()) {
    return;
  }

  wstringstream ss(message);
  wstring word;
  wstring buffer;
  const size_t WORDS_PER_CHUNK = 5;
  size_t wordCount = 0;

  while (ss >> word) {
    buffer += word + L" ";
    wordCount++;

    if (wordCount >= WORDS_PER_CHUNK) {
      Write(buffer);
      buffer.clear();
      wordCount = 0;
      Sleep(100); // Shorter delay for word-based streaming
    }
  }

  // Write remaining buffer
  if (!buffer.empty()) {
    Write(buffer);
  }
}

// Alternative: Stream by lines (best for formatted text)
void MCPClient::StreamByLines(const wstring &message) {
  if (!s_pWordApp) {
    MSGBOX_ERROR(L"No Word Application available");
    return;
  }

  if (message.empty()) {
    return;
  }

  wstringstream ss(message);
  wstring line;

  while (getline(ss, line)) {
    line += L"\n"; // Preserve newline
    Write(line);
    Sleep(100); // Delay between lines
  }
}

vector<wstring> MCPClient::SplitIntoChunks(const wstring &text,
                                           size_t chunkSize) {
  vector<wstring> chunks;

  for (size_t i = 0; i < text.length(); i += chunkSize) {
    chunks.push_back(text.substr(i, min(chunkSize, text.length() - i)));
  }

  return chunks;
}

} // namespace MCPHelper