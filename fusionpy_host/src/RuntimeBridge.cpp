#include "RuntimeBridge.h"
#include <ShlObj.h>
#include <fstream>
#include <filesystem>

using namespace fusionpy;

RuntimeBridge::RuntimeBridge() = default;

bool RuntimeBridge::openLog(const std::wstring &path) {
    m_logHandle = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m_logHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    SetFilePointer(m_logHandle, 0, nullptr, FILE_END);
    return true;
}

void RuntimeBridge::writeDebugger(const std::wstring &line) {
    OutputDebugStringW(line.c_str());
    if (m_logHandle != INVALID_HANDLE_VALUE) {
        std::wstring msg = line + L"\r\n";
        DWORD written = 0;
        WriteFile(m_logHandle, msg.c_str(), static_cast<DWORD>(msg.size() * sizeof(wchar_t)), &written, nullptr);
    }
}

bool RuntimeBridge::initialise(const RuntimeConfig &config) {
    if (!openLog(config.logFile)) {
        return false;
    }
    writeDebugger(L"[fusionpy_host] Initialising runtime bridge for " + config.extensionName);

    if (!enforceCapabilities(config.capabilities)) {
        writeDebugger(L"[fusionpy_host] Capability enforcement failed");
        return false;
    }

    m_pythonHome = config.venvRoot;
    return true;
}

void RuntimeBridge::shutdown() {
    writeDebugger(L"[fusionpy_host] Shutdown requested");
    if (m_logHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_logHandle);
        m_logHandle = INVALID_HANDLE_VALUE;
    }
}

bool RuntimeBridge::launchPython() {
    if (m_pythonHome.empty()) {
        return false;
    }
    if (SetEnvironmentVariableW(L"PYTHONHOME", m_pythonHome.c_str()) == FALSE) {
        writeDebugger(L"[fusionpy_host] Failed to set PYTHONHOME");
        return false;
    }
    writeDebugger(L"[fusionpy_host] PYTHONHOME configured");
    return true;
}

bool RuntimeBridge::enforceCapabilities(const CapabilityRequest &requested) {
    // Capability enforcement can be extended. For now, log the requested set so that
    // the host has a consistent surface that can be tightened later.
    std::wstring requestedFlags;
    if (requested.filesystem) requestedFlags += L" filesystem";
    if (requested.network) requestedFlags += L" network";
    if (requested.registry) requestedFlags += L" registry";
    if (requestedFlags.empty()) requestedFlags = L" none";
    writeDebugger(L"[fusionpy_host] Requested capabilities:" + requestedFlags);
    return true;
}

std::wstring RuntimeBridge::getPythonHome() const {
    return m_pythonHome;
}

static RuntimeBridge g_bridge;

extern "C" __declspec(dllexport) BOOL __stdcall FusionPy_Startup(const wchar_t *manifestPath) {
    std::filesystem::path manifest{manifestPath};
    RuntimeConfig cfg{};
    cfg.extensionName = manifest.stem().wstring();
    cfg.manifestPath = manifest.wstring();
    cfg.venvRoot = manifest.parent_path() / L"venv";
    cfg.logFile = manifest.parent_path() / L"fusionpy_host.log";
    cfg.capabilities = CapabilityRequest{};
    return g_bridge.initialise(cfg) && g_bridge.launchPython();
}

extern "C" __declspec(dllexport) void __stdcall FusionPy_Shutdown() {
    g_bridge.shutdown();
}
