#include "driverNamespace.h"


static DWORD getProcessId(const wchar_t* processName) {
    DWORD processId{};

    HANDLE snapShot{ CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL) };
    if (snapShot == INVALID_HANDLE_VALUE)
        return processId;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(decltype(entry));

    if (Process32FirstW(snapShot, &entry) == TRUE) {
        // Check if the first handle is the one we want
        if (_wcsicmp(processName, entry.szExeFile) == 0)
            processId = entry.th32ProcessID;
        else {
            while (Process32NextW(snapShot, &entry) == true) {
                if (_wcsicmp(processName, entry.szExeFile) == 0) {
                    processId = entry.th32ProcessID;
                    break;
                }
            }
        }
    }
    CloseHandle(snapShot);
    return processId;
}

static std::uintptr_t getModuleBase(const DWORD pid, const wchar_t* moduleName) {
    std::uintptr_t moduleBase{};

    HANDLE snapShot{ CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid) };
    if (snapShot == INVALID_HANDLE_VALUE)
        return moduleBase;

    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(decltype(entry));

    if (Module32FirstW(snapShot, &entry) == TRUE) {
        // Check if the first handle is the one we want
        if (wcsstr(moduleName, entry.szModule) != nullptr)
            moduleBase = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
        else {
            while (Module32NextW(snapShot, &entry) == true) {
                if (wcsstr(moduleName, entry.szModule) != nullptr) {
                    moduleBase = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
                    break;
                }
            }
        }
    }
    CloseHandle(snapShot);
    return moduleBase;
}






int main() {
    const DWORD pid = getProcessId(L"notepad.exe");

    if (pid == 0) {
        std::cout << "Failed to find notepad\n";
        std::cin.get();
        return 1;
    }

    const HANDLE driverHandle{ CreateFile(L"\\\\.\\broDriver", GENERIC_READ, 0, nullptr, 
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };

    if (driverHandle == INVALID_HANDLE_VALUE) {
        std::cout << "Failed to create out driver handle.\n";
        std::cin.get();
        return 1;
    }

    if (driver::attachToProcess(driverHandle, pid) == true)
        std::cout << "Attachment successful.\n";

    CloseHandle(driverHandle);
    std::cin.get();
    return 0;
}