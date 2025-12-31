#include <Windows.h>
#include <string>
#include <iostream>

bool LaunchFusionWithBridge(const std::wstring &fusionPath, const std::wstring &manifestPath);

int wmain(int argc, wchar_t **argv) {
    if (argc < 3) {
        std::wcerr << L"Usage: fusionpy_injector <Fusion.exe path> <manifest.mfx.json>" << std::endl;
        return 1;
    }
    if (!LaunchFusionWithBridge(argv[1], argv[2])) {
        std::wcerr << L"Failed to start Fusion with fusionpy host bridge" << std::endl;
        return 1;
    }
    return 0;
}
