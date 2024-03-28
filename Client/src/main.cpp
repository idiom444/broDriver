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
    const DWORD pid = getProcessId(L"cs2.exe");

    if (pid == 0) {
        std::cout << "Failed to find cs2\n";
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

    if (driver::attachToProcess(driverHandle, pid) == true) {
        std::cout << "Attachment successful.\n";
        if (const std::uintptr_t client{ getModuleBase(pid, L"client.dll") }; client != 0) {
            std::cout << "Client found.\n";

            while (true){
                if (GetAsyncKeyState(VK_END))
                    break;

                    const auto locaPlayerPawn = driver::readMemory<std::uintptr_t>(
                        driverHandle, client + client_dll::dwLocalPlayerPawn);

                    if (locaPlayerPawn == 0)
                        continue;

                    const auto flags = driver::readMemory<std::uint32_t>(driverHandle,
                        locaPlayerPawn + C_BaseEntity::m_fFlags);
                    const bool inAir = flags & (1 << 0);
                    const bool spacePressed = GetAsyncKeyState(VK_SPACE);
                    const auto forceJump = driver::readMemory<DWORD>(driverHandle, client + client_dll::dwForceJump);

                    if (spacePressed && inAir) {
                        driver::writeMemory(driverHandle, client + client_dll::dwForceJump, 65537);
                    }
                    else if (spacePressed && !inAir) {
                        driver::writeMemory(driverHandle, client + client_dll::dwForceJump, 256);
                    }
                    else if (!spacePressed && forceJump == 66537) {
                        driver::writeMemory(driverHandle, client + client_dll::dwForceJump, 256);
                    }
            }
        }
    }
    CloseHandle(driverHandle);
    std::cin.get();
    return 0;
}