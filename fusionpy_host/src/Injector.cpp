#include "RuntimeBridge.h"
#include <Windows.h>
#include <string>
#include <filesystem>

using namespace fusionpy;

bool LaunchFusionWithBridge(const std::wstring &fusionPath, const std::wstring &manifestPath) {
    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    std::wstring cmd = L"\"" + fusionPath + L"\"";
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &si, &pi)) {
        return false;
    }

    // Minimal DLL injection to ensure the host loads at process start.
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    auto pLoadLibraryW = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(hKernel32, "LoadLibraryW"));
    if (!pLoadLibraryW) {
        TerminateProcess(pi.hProcess, 1);
        return false;
    }

    std::filesystem::path hostPath = std::filesystem::path(manifestPath).parent_path() / L"fusionpy_host.dll";
    std::wstring widePath = hostPath.wstring();
    LPVOID remoteMem = VirtualAllocEx(pi.hProcess, nullptr, (widePath.size() + 1) * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        TerminateProcess(pi.hProcess, 1);
        return false;
    }
    WriteProcessMemory(pi.hProcess, remoteMem, widePath.c_str(), (widePath.size() + 1) * sizeof(wchar_t), nullptr);

    HANDLE hThread = CreateRemoteThread(pi.hProcess, nullptr, 0, pLoadLibraryW, remoteMem, 0, nullptr);
    if (!hThread) {
        TerminateProcess(pi.hProcess, 1);
        return false;
    }
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    // Let the bridge start up using the manifest specified.
    HMODULE remoteModule = nullptr;
    GetExitCodeThread(hThread, reinterpret_cast<DWORD *>(&remoteModule));
    if (remoteModule) {
        auto remoteStartup = reinterpret_cast<BOOL(WINAPI *)(const wchar_t *)>(GetProcAddress(remoteModule, "FusionPy_Startup"));
        if (remoteStartup) {
            remoteStartup(manifestPath.c_str());
        }
    }

    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}
