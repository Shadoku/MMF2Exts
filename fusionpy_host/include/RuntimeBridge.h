#pragma once

#include <string>
#include <vector>
#include <optional>
#include <Windows.h>

namespace fusionpy {

struct CapabilityRequest {
    bool filesystem{false};
    bool network{false};
    bool registry{false};
};

struct RuntimeConfig {
    std::wstring extensionName;
    std::wstring manifestPath;
    std::wstring venvRoot;
    std::wstring logFile;
    CapabilityRequest capabilities;
};

class RuntimeBridge {
public:
    RuntimeBridge();
    bool initialise(const RuntimeConfig &config);
    void shutdown();
    bool launchPython();
    void log(const std::wstring &message, int level = 1);
    bool enforceCapabilities(const CapabilityRequest &requested);
    std::wstring getPythonHome() const;

private:
    std::wstring m_pythonHome;
    HANDLE m_logHandle{INVALID_HANDLE_VALUE};
    bool openLog(const std::wstring &path);
    void writeDebugger(const std::wstring &line);
};

} // namespace fusionpy

extern "C" __declspec(dllexport) BOOL __stdcall FusionPy_Startup(const wchar_t *manifestPath);
extern "C" __declspec(dllexport) void __stdcall FusionPy_Shutdown();
