#pragma once

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"

#define _ATL_APARTMENT_THREADED

#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW

#include "resource.h"
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <atltypes.h>
#include <atlwin.h>

// using namespace HelperDatabase;

// namespace HandlerDatabaseCore {
// class DatabaseCore {
// public:
//   Database *dbHelper = nullptr;

//   DatabaseCore() {
//     LOG("Database Core Initialized", LogLevel::INFO);
//     dbHelper = new Database();
//     dbHelper->LoadDb();
//     dbHelper->ShowTables();
//   }

//   ~DatabaseCore() { dbHelper->CloseDb(); }
// };
// } // namespace HandlerDatabaseCore

// ============================================================================
// Office Type Library Imports (Office 2010)
// ============================================================================

// Office core library (MSO.DLL) - provides ICustomTaskPaneConsumer
#import "C:\Program Files (x86)\Common Files\Microsoft Shared\OFFICE14\MSO.DLL" rename_namespace("Office") raw_interfaces_only, named_guids, exclude("wireHWND", "_RemotableHandle", "__MIDL_IWinTypes_0009")

// Extensibility library - provides IDTExtensibility2
// This is typically in the same location or in VSTools
#import "libid:AC0714F2-3D04-11D1-AE7D-00A0C90F26F4" rename_namespace("AddInDesignerObjects") raw_interfaces_only, named_guids, auto_search
