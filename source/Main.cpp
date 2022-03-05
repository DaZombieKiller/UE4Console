#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <DbgHelp.h>
#include <Psapi.h>
#include <detours.h>
#include <stdlib.h>
#include <stdio.h>

#include "UConsole.h"
#include "UGameViewportClient.h"
#include "FOutputDeviceRedirector.h"
#include "FStaticConstructObjectParameters.h"

// TODO: Attempt to restore UConsole::BuildRuntimeAutoCompleteList.

DWORD(WINAPI *EntryPoint)(LPVOID);

UObject*(*StaticConstructObject_Internal)(FStaticConstructObjectParameters*);

FOutputDeviceRedirector*(*GetGlobalLogSingleton)(void);

void*(*AddConsoleObject)(void*, wchar_t const*, void*);

ULocalPlayer* SetupInitialLocalPlayerDetour(UGameViewportClient* Self, FString* OutError)
{
    FStaticConstructObjectParameters Params = { 0 };
    Params.Outer = Self;
    Params.Class = UConsole::GetPrivateStaticClass();

    // TODO: Discover the offset of ViewportConsole dynamically using SymGetTypeInfo.
    Self->ViewportConsole = (UConsole*)StaticConstructObject_Internal(&Params);
#if 0
    // For some reason, this causes the game to crash on exit.
    FOutputDeviceRedirector::AddOutputDevice(GetGlobalLogSingleton(), self->ViewportConsole);
#endif
    return UGameViewportClient::SetupInitialLocalPlayer(Self, OutError);
}

void* AddConsoleObjectDetour(void* Self, wchar_t const* Name, void* Obj)
{
    printf("%ls\n", Name);
    return AddConsoleObject(Self, Name, Obj);
}

DWORD WINAPI EntryPointDetour(LPVOID lpParam)
{
#if UE4CONSOLE_LOG_FILE
    (void)freopen("stdout.txt", "w", stdout);
#endif
    HANDLE hProcess = GetCurrentProcess();
    HMODULE hModule = GetModuleHandleW(nullptr);

    // Retrieve the file path twice, truncate the second one
    // so that it refers to the containing directory.
    WCHAR szFilePath[MAX_PATH];
    WCHAR szDirectory[MAX_PATH];
    GetModuleFileNameW(hModule, szFilePath, MAX_PATH);
    GetModuleFileNameW(hModule, szDirectory, MAX_PATH);
    *wcsrchr(szDirectory, L'\\') = L'\0';

    // Retrieve the module information to provide to SymLoadModuleExW.
    MODULEINFO moduleInfo;
    GetModuleInformation(hProcess, hModule, &moduleInfo, sizeof(moduleInfo));

    // Initialize the DbgHelp API.
    // We pass FALSE for fInvadeProcess so that DbgHelp doesn't load the modules yet.
    SymSetOptions(SYMOPT_IGNORE_CVREC | SYMOPT_LOAD_ANYTHING);
    SymInitializeW(hProcess, szDirectory, FALSE);

    // Locate the PDB file manually, rather than allowing DbgHelp to handle it.
    // This handles the case where the PDB is named after the executable despite
    // the embedded debug information expecting the PDB under a different name.
    WCHAR szPdbName[MAX_PATH];
    WCHAR szPdbPath[MAX_PATH];
    SymGetSymbolFileW(hProcess, nullptr, szFilePath, sfPdb, szPdbName, MAX_PATH, nullptr, 0);

    // We need an absolute PDB file path, but we might have a relative one (or just a file name).
    // The PDB should be next to the executable, so we also need to change the current directory
    // temporarily so that GetFullPathNameW will treat a non-absolute path as relative to the exe.
    WCHAR szCurrentDirectory[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, szCurrentDirectory);
    SetCurrentDirectoryW(szDirectory);
    GetFullPathNameW(szPdbName, MAX_PATH, szPdbPath, nullptr);
    SetCurrentDirectoryW(szCurrentDirectory);

    // Now we can load the PDB.
    if (SymLoadModuleExW(hProcess, nullptr, szPdbPath, nullptr, (DWORD64)hModule, moduleInfo.SizeOfImage, nullptr, 0) == 0)
    {
        // If the load fails, just give up and let the game continue booting.
        goto Cleanup;
    }

    // Initialize the SYMBOL_INFOW structure.
    SYMBOL_INFOW Symbol;
    Symbol.SizeOfStruct = sizeof(SYMBOL_INFOW);

    // Locate all of the symbols we need.
    SymFromNameW(hProcess, L"?StaticConstructObject_Internal@@YAPEAVUObject@@AEBUFStaticConstructObjectParameters@@@Z", &Symbol);
    *(LPVOID*)&StaticConstructObject_Internal = (LPVOID)Symbol.Address;

    SymFromNameW(hProcess, L"?GetPrivateStaticClass@UConsole@@CAPEAVUClass@@XZ", &Symbol);
    *(LPVOID*)&UConsole::GetPrivateStaticClass = (LPVOID)Symbol.Address;

#if 0
    SymFromNameW(hProcess, L"?GetGlobalLogSingleton@@YAPEAVFOutputDeviceRedirector@@XZ", &Symbol);
    *(LPVOID*)&GetGlobalLogSingleton = (LPVOID)Symbol.Address;

    SymFromNameW(hProcess, L"?AddOutputDevice@FOutputDeviceRedirector@@QEAAXPEAVFOutputDevice@@@Z", &Symbol);
    *(LPVOID*)&FOutputDeviceRedirector::AddOutputDevice = (LPVOID)Symbol.Address;
#endif

#if UE4CONSOLE_LOG_COMMANDS
    SymFromNameW(hProcess, L"?AddConsoleObject@FConsoleManager@@AEAAPEAVIConsoleObject@@PEB_WPEAV2@@Z", &Symbol);
    *(LPVOID*)&AddConsoleObject = (LPVOID)Symbol.Address;
#endif

    SymFromNameW(hProcess, L"?SetupInitialLocalPlayer@UGameViewportClient@@UEAAPEAVULocalPlayer@@AEAVFString@@@Z", &Symbol);
    *(LPVOID*)&UGameViewportClient::SetupInitialLocalPlayer = (LPVOID)Symbol.Address;

    // And now we can apply our detour.
    DetourTransactionBegin();
#if UE4CONSOLE_LOG_COMMANDS
    DetourAttach((LPVOID*)&AddConsoleObject, &AddConsoleObjectDetour);
#endif
    DetourAttach((LPVOID*)&UGameViewportClient::SetupInitialLocalPlayer, &SetupInitialLocalPlayerDetour);
    DetourTransactionCommit();

Cleanup:
    // Now that we've retrieved the symbols, we don't need DbgHelp anymore.
    SymCleanup(hProcess);

    // And allow the program to continue its initialization.
    return EntryPoint(lpParam);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // Locate the NT header so we can find the entry point address.
        PBYTE ImageBase = (PBYTE)GetModuleHandle(nullptr);
        PIMAGE_NT_HEADERS NtHeader = ImageNtHeader(ImageBase);

        // The entry point is the load address + AddressOfEntryPoint.
        *(LPVOID*)&EntryPoint = ImageBase + NtHeader->OptionalHeader.AddressOfEntryPoint;

        // Detour the entry point so we can hook things as early as possible.
        // It is necessary to do this instead of locating the symbols here
        // because DllMain is executed while the loader lock is active.
        DetourTransactionBegin();
        DetourAttach((LPVOID*)&EntryPoint, &EntryPointDetour);
        DetourTransactionCommit();
    }
    
    return TRUE;
}
