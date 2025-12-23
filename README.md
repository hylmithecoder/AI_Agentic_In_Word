# 🤖 AgenticAIOnWord

A powerful Microsoft Word 2010 Add-in that brings **Agentic AI capabilities** directly into your document workflow. Built with C++/ATL and designed for seamless integration with AI services via the Model Context Protocol (MCP).

![Version](https://img.shields.io/badge/version-0.1.0-blue)
![Word](https://img.shields.io/badge/Word-2010-green)
![Platform](https://img.shields.io/badge/platform-Windows%20x86-lightgrey)

---

## ✨ Features

- 🎯 **Task Pane UI** - Modern, elegant interface integrated into Word
- 🗄️ **SQLite Database** - Local storage for settings, history, and context
- 🤖 **MCP Client** - Connect to AI servers via Model Context Protocol
- ⚡ **High Performance** - Native C++ with optional Zig components

---

## 📁 Project Structure

```
AgenticAIOnWord/
├── 📂 include/           # Header files (.h)
│   ├── framework.h       # ATL/Office framework includes
│   ├── Connect.h         # Add-in connection handler
│   ├── TaskPaneControl.h # Task pane UI control
│   └── ...
│
├── 📂 src/               # Source files (.cpp)
│   ├── Connect.cpp       # IDTExtensibility2 implementation
│   ├── TaskPaneControl.cpp # UI implementation
│   └── ...
│
├── 📂 res/               # Resources
│   ├── AgenticAIOnWord.rc  # Resource script
│   ├── *.rgs              # Registry scripts
│   └── AgenticAIOnWord.def # Module definition
│
├── 📂 idl/               # Interface definitions
│   └── AgenticAIOnWord.idl
│
├── 📂 scripts/           # Helper scripts
│   └── RegisterWordAddin.reg
│
├── CMakeLists.txt        # CMake build configuration
└── AgenticAIOnWord.vcxproj # Visual Studio project
```

---

## 🛠️ Build Requirements

- **Visual Studio 2022** with C++ desktop development
- **Windows SDK 10.0.19041.0** or later
- **Microsoft Office 2010** (32-bit) or later
- **CMake 3.20+** (optional, for CMake builds)

---

## 🚀 Quick Start

### Build
```powershell
# Using MSBuild (recommended)
msbuild AgenticAIOnWord.vcxproj /p:Configuration=Debug /p:Platform=Win32

# Or using CMake
cmake -B build -A Win32
cmake --build build --config Debug
```

### Register
```cmd
# Run as Administrator
regsvr32 "Debug\AgenticAIOnWord.dll"
```

### Activate in Word
1. Open Word 2010
2. Go to **File → Options → Add-ins**
3. Select **COM Add-ins** → **Go...**
4. Check **Agentic AI Assistant**
5. Click **OK**

---

## 🗺️ Roadmap

### Phase 1: Database Integration 🗄️
- [ ] Integrate **SQLite** for local storage
- [ ] Store user preferences and settings
- [ ] Cache conversation history
- [ ] Document context persistence

### Phase 2: Elegant UI ✨
- [ ] Modern Task Pane design with GDI+
- [ ] Dark/Light theme support
- [ ] Smooth animations and transitions
- [ ] Rich text input with markdown preview
- [ ] Responsive layout

### Phase 3: MCP Integration 🔌
- [ ] Implement **MCP Client** protocol
- [ ] Connect to MCP-compatible AI servers
- [ ] Tool calling and resource management
- [ ] Streaming responses

### Phase 4: Zig MCP Bridge ⚡
- [ ] High-performance MCP client in **Zig**
- [ ] Async networking with minimal overhead
- [ ] C/C++ interop via Zig's C ABI
- [ ] Cross-platform potential

### Phase 5: Advanced Features 🚀
- [ ] Document analysis and summarization
- [ ] Smart formatting assistance
- [ ] Multi-language support
- [ ] Voice commands (optional)

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Microsoft Word 2010                      │
├─────────────────────────────────────────────────────────────┤
│                   AgenticAIOnWord Add-in                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  Task Pane  │  │   Connect   │  │    Word Object      │  │
│  │     UI      │◄─┤   Handler   │──┤      Model          │  │
│  └──────┬──────┘  └─────────────┘  └─────────────────────┘  │
│         │                                                    │
│  ┌──────▼──────┐  ┌─────────────┐                           │
│  │   SQLite    │  │ MCP Client  │◄──── Zig Bridge           │
│  │   Storage   │  │  (C++/Zig)  │                           │
│  └─────────────┘  └──────┬──────┘                           │
└────────────────────────────┼────────────────────────────────┘
                             │
                    ┌────────▼────────┐
                    │   MCP Server    │
                    │ (Claude, GPT,   │
                    │  Local LLM...)  │
                    └─────────────────┘
```

---

## 📚 Technology Stack

| Component | Technology |
|-----------|------------|
| Core Add-in | C++ / ATL / COM |
| UI Framework | Win32 GDI+ / Direct2D |
| Database | SQLite 3 |
| AI Protocol | Model Context Protocol (MCP) |
| High-perf Bridge | Zig |
| Build System | MSBuild / CMake |

---

## 🔧 Development

### Adding SQLite Support
```cpp
#include <sqlite3.h>

// Initialize database
sqlite3* db;
sqlite3_open("agenticai.db", &db);

// Create tables
sqlite3_exec(db, R"(
    CREATE TABLE IF NOT EXISTS settings (
        key TEXT PRIMARY KEY,
        value TEXT
    );
    CREATE TABLE IF NOT EXISTS history (
        id INTEGER PRIMARY KEY,
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
        role TEXT,
        content TEXT
    );
)", nullptr, nullptr, nullptr);
```

### MCP Client Interface (Planned)
```cpp
class IMCPClient {
public:
    virtual HRESULT Connect(LPCWSTR serverUrl) = 0;
    virtual HRESULT SendMessage(LPCWSTR message, IMCPCallback* callback) = 0;
    virtual HRESULT CallTool(LPCWSTR toolName, LPCWSTR args) = 0;
    virtual HRESULT Disconnect() = 0;
};
```

---

## 📄 License

MIT License - See [LICENSE](LICENSE) for details.

---

## 🤝 Contributing

Contributions are welcome! Please read the contributing guidelines before submitting PRs.

---

<p align="center">
  Made with ❤️ for enhanced document productivity
</p>
