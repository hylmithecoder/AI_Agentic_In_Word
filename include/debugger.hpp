#pragma once
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <string>
#include <windows.h>
using namespace std;

namespace Debug {
enum class LogLevel { INFO, WARNING, CRASH, SUCCESS };

// ANSI escape codes untuk colors
static const char *getColorCode(LogLevel level) {
  switch (level) {
  case LogLevel::INFO:
    return "\033[1;34m"; // Blue
  case LogLevel::WARNING:
    return "\033[1;33m"; // Yellow
  case LogLevel::CRASH:
    return "\033[1;31m"; // Red
  case LogLevel::SUCCESS:
    return "\033[1;32m"; // Green
  default:
    return "\033[0m"; // Reset
  }
}

static const char *getLevelString(LogLevel level) {
  switch (level) {
  case LogLevel::INFO:
    return "[INFO]";
  case LogLevel::WARNING:
    return "[WARNING]";
  case LogLevel::CRASH:
    return "[CRASH]";
  case LogLevel::SUCCESS:
    return "[SUCCESS]";
  default:
    return "[LOG]";
  }
}

// Internal function untuk handle format string
static void LogFormattedWithLocation(const char *format, LogLevel level,
                                     const char *file, int line, va_list args) {
  const char *colorCode = getColorCode(level);
  const char *levelStr = getLevelString(level);
  const char *reset = "\033[0m";

  // Print level dengan warna
  cout << colorCode << levelStr << " ";

  // Format dan print message dengan arguments
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  cout << buffer << " (" << file << ":" << line << ")" << reset << endl;
}

// Overload tanpa arguments
static void LogWithLocation(const char *format, LogLevel level,
                            const char *file, int line) {
  const char *colorCode = getColorCode(level);
  const char *levelStr = getLevelString(level);
  const char *reset = "\033[0m";

  cout << colorCode << levelStr << " " << format << " (" << file << ":" << line
       << ")" << reset << endl;
}

static void Log(const string &message, LogLevel level = LogLevel::INFO) {
  const char *colorCode = getColorCode(level);
  const char *levelStr = getLevelString(level);
  const char *reset = "\033[0m";

  cout << colorCode << levelStr << " ";
  cout << message << reset << endl;
}

// Variadic template version untuk mendukung format string dengan arguments
template <typename... Args>
static void Log(const char *format, LogLevel level, Args... args) {
  const char *colorCode = getColorCode(level);
  const char *levelStr = getLevelString(level);
  const char *reset = "\033[0m";

  cout << colorCode << levelStr << " ";

  char buffer[1024];
  snprintf(buffer, sizeof(buffer), format, args...);
  cout << buffer << reset << endl;
}

// Variadic template untuk format tanpa level (default INFO)
template <typename... Args> static void Log(const char *format, Args... args) {
  Log(format, LogLevel::INFO, args...);
}

template <typename T>
static void LogPointer(const string &name, T handle,
                       LogLevel level = LogLevel::INFO) {
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%s | Type: %s | Address: %p | Decimal: %lu",
           name.c_str(), typeid(handle).name(), (void *)handle,
           (unsigned long)handle);
  Log(buffer, level);
}

// ============================================
// MACRO DEFINITIONS dengan automatic file/line
// ============================================

#define LOG(format, level)                                                     \
  do {                                                                         \
    const char *colorCode = Debug::getColorCode((level));                      \
    const char *levelStr = Debug::getLevelString((level));                     \
    const char *reset = "\033[0m";                                             \
    cout << colorCode << levelStr << " ";                                      \
    char buffer[1024];                                                         \
    snprintf(buffer, sizeof(buffer), (format));                                \
    cout << buffer << " (" << __FILE__ << ":" << __LINE__ << ")" << reset      \
         << endl;                                                              \
  } while (0)

// Macro untuk simple string logging
#define DEBUG_LOG(format, ...)                                                 \
  do {                                                                         \
    const char *colorCode = Debug::getColorCode((Debug::LogLevel::INFO));      \
    const char *levelStr = Debug::getLevelString((Debug::LogLevel::INFO));     \
    const char *reset = "\033[0m";                                             \
    cout << colorCode << levelStr << " ";                                      \
    char buffer[1024];                                                         \
    snprintf(buffer, sizeof(buffer), (format), ##__VA_ARGS__);                 \
    cout << buffer << " (" << __FILE__ << ":" << __LINE__ << ")" << reset      \
         << endl;                                                              \
  } while (0)

// Macro untuk format string dengan arguments
#define DEBUG_LOGF(format, level, ...)                                         \
  do {                                                                         \
    const char *colorCode = Debug::getColorCode((level));                      \
    const char *levelStr = Debug::getLevelString((level));                     \
    const char *reset = "\033[0m";                                             \
    cout << colorCode << levelStr << " ";                                      \
    char buffer[1024];                                                         \
    snprintf(buffer, sizeof(buffer), (format), ##__VA_ARGS__);                 \
    cout << buffer << " (" << __FILE__ << ":" << __LINE__ << ")" << reset      \
         << endl;                                                              \
  } while (0)

// Macro untuk pointer logging
#define DEBUG_LOG_POINTER(name, handle, level)                                 \
  do {                                                                         \
    char buffer[256];                                                          \
    snprintf(buffer, sizeof(buffer),                                           \
             "%s | Type: %s | Address: %p | Decimal: %lu", (name),             \
             typeid(handle).name(), (void *)(handle),                          \
             (unsigned long)(handle));                                         \
    Debug::LogWithLocation(buffer, (level), __FILE__, __LINE__);               \
  } while (0)

// Macro untuk error checking (seperti VK_CHECK_RESULT)
#define DEBUG_ASSERT(condition, message, level)                                \
  do {                                                                         \
    if (!(condition)) {                                                        \
      const char *colorCode = Debug::getColorCode((level));                    \
      const char *levelStr = Debug::getLevelString((level));                   \
      const char *reset = "\033[0m";                                           \
      cout << colorCode << levelStr << " ASSERTION FAILED: " << (message)      \
           << " (" << __FILE__ << ":" << __LINE__ << ")" << reset << endl;     \
      assert((condition));                                                     \
    }                                                                          \
  } while (0)

// ============================================
// WINDOW MESSAGE BOX MACROS
// ============================================

static void ShowMsgBoxWithLocation(const wchar_t *title,
                                   const std::wstring &message,
                                   const char *file, int line, UINT type) {
  // Convert filename to wide char
  wchar_t wFile[1024];
  size_t converted = 0;
  mbstowcs_s(&converted, wFile, sizeof(wFile) / sizeof(wchar_t), file,
             _TRUNCATE);

  // Convert line number
  wchar_t wLine[32];
  swprintf_s(wLine, sizeof(wLine) / sizeof(wchar_t), L"%d", line);

  // Combine
  std::wstring fullMessage = message + L"\n\n(" + wFile + L":" + wLine + L")";

  // Show
  MessageBoxW(NULL, fullMessage.c_str(), title, type);
}

// ============================================
// WINDOW MESSAGE BOX MACROS
// ============================================

// Info message box (blue icon)
#define MSGBOX_INFO(title, message)                                            \
  Debug::ShowMsgBoxWithLocation((title), (std::wstring)(message), __FILE__,    \
                                __LINE__, MB_OK | MB_ICONINFORMATION)

// Success message box (green checkmark)
#define MSGBOX_SUCCESS(title, message)                                         \
  Debug::ShowMsgBoxWithLocation((title), (std::wstring)(message), __FILE__,    \
                                __LINE__, MB_OK | MB_ICONINFORMATION)

// Warning message box (yellow exclamation)
#define MSGBOX_WARNING(title, message)                                         \
  Debug::ShowMsgBoxWithLocation((title), (std::wstring)(message), __FILE__,    \
                                __LINE__, MB_OK | MB_ICONWARNING)

// Error/Crash message box (red X)
#define MSGBOX_ERROR(title, message)                                           \
  Debug::ShowMsgBoxWithLocation((title), (std::wstring)(message), __FILE__,    \
                                __LINE__, MB_OK | MB_ICONERROR)

// Crash message box with abort option
#define MSGBOX_CRASH(title, message)                                           \
  do {                                                                         \
    Debug::ShowMsgBoxWithLocation((title), (std::wstring)(message), __FILE__,  \
                                  __LINE__,                                    \
                                  MB_ABORTRETRYIGNORE | MB_ICONERROR);         \
    abort();                                                                   \
  } while (0)

// Format message box with printf-style formatting
#define MSGBOX_INFOF(title, format, ...)                                       \
  do {                                                                         \
    wchar_t buffer[1024];                                                      \
    swprintf_s(buffer, sizeof(buffer) / sizeof(wchar_t), (format),             \
               ##__VA_ARGS__);                                                 \
    MessageBoxW(NULL, buffer, (title), MB_OK | MB_ICONINFORMATION);            \
  } while (0)

#define MSGBOX_WARNINGF(title, format, ...)                                    \
  do {                                                                         \
    wchar_t buffer[1024];                                                      \
    swprintf_s(buffer, sizeof(buffer) / sizeof(wchar_t), (format),             \
               ##__VA_ARGS__);                                                 \
    MessageBoxW(NULL, buffer, (title), MB_OK | MB_ICONWARNING);                \
  } while (0)

#define MSGBOX_ERRORF(title, format, ...)                                      \
  do {                                                                         \
    wchar_t buffer[1024];                                                      \
    swprintf_s(buffer, sizeof(buffer) / sizeof(wchar_t), (format),             \
               ##__VA_ARGS__);                                                 \
    MessageBoxW(NULL, buffer, (title), MB_OK | MB_ICONERROR);                  \
  } while (0)

// ANSI string version (converts to wide string automatically)
#define MSGBOX_INFO_A(title, message)                                          \
  do {                                                                         \
    MessageBoxA(NULL, (message), (title), MB_OK | MB_ICONINFORMATION);         \
  } while (0)

#define MSGBOX_SUCCESS_A(title, message)                                       \
  do {                                                                         \
    MessageBoxA(NULL, (message), (title), MB_OK | MB_ICONINFORMATION);         \
  } while (0)

#define MSGBOX_WARNING_A(title, message)                                       \
  do {                                                                         \
    MessageBoxA(NULL, (message), (title), MB_OK | MB_ICONWARNING);             \
  } while (0)

#define MSGBOX_ERROR_A(title, message)                                         \
  do {                                                                         \
    MessageBoxA(NULL, (message), (title), MB_OK | MB_ICONERROR);               \
  } while (0)

} // namespace Debug