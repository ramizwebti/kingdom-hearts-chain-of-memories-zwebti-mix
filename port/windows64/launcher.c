/* Native, Unicode-safe entry point for the extracted Windows package.
 * The PowerShell launcher retains ROM validation/import and save ownership. */
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <wchar.h>
#include <stdio.h>

enum { PATH_CAP = 32768, COMMAND_CAP = 65536 };

static int Fail(const wchar_t* message) {
    MessageBoxW(NULL, message, L"Kingdom Hearts Chain of Memories: Zwebti Mix", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    return 1;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR arguments, int show) {
    wchar_t packageRoot[PATH_CAP], script[PATH_CAP], powershell[PATH_CAP];
    wchar_t command[COMMAND_CAP];
    wchar_t readyName[80];
    wchar_t* separator;
    DWORD length, result, attributes;
    HANDLE readyEvent;
    STARTUPINFOW startup = {0};
    PROCESS_INFORMATION process = {0};
    (void)instance; (void)previous; (void)arguments; (void)show;

    length = GetModuleFileNameW(NULL, packageRoot, PATH_CAP);
    if (!length || length >= PATH_CAP)
        return Fail(L"Kingdom Hearts Chain of Memories: Zwebti Mix could not locate its game folder. Extract the entire ZIP into a folder such as Downloads, then open Kingdom Hearts Chain of Memories - Zwebti Mix.exe there.");
    separator = wcsrchr(packageRoot, L'\\');
    if (!separator)
        return Fail(L"Kingdom Hearts Chain of Memories: Zwebti Mix could not locate its game folder. Extract the entire ZIP, then open Kingdom Hearts Chain of Memories - Zwebti Mix.exe from the extracted folder.");
    separator[1] = L'\0';
    if (swprintf(script, PATH_CAP, L"%lslaunch.ps1", packageRoot) < 0)
        return Fail(L"The game folder path is too long. Extract the ZIP into a shorter location such as Downloads.");
    attributes = GetFileAttributesW(script);
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY))
        return Fail(L"The package needs to be extracted first.\n\nRight-click the downloaded ZIP, choose Extract All, then Extract. Open the extracted folder and double-click Kingdom Hearts Chain of Memories - Zwebti Mix.exe. Keep launch.ps1 and the runtime folder beside it.");

    length = GetSystemDirectoryW(powershell, PATH_CAP);
    if (!length || length >= PATH_CAP ||
        swprintf(powershell + length, PATH_CAP - length,
                 L"\\WindowsPowerShell\\v1.0\\powershell.exe") < 0)
        return Fail(L"Windows PowerShell could not be located. Open START_HERE.html for launch help, or try PLAY.cmd from the extracted folder.");
    /* File names cannot contain quotes on Windows. Both quoted paths end in
     * a file name, so no trailing-backslash escape ambiguity is possible. */
    if (swprintf(command, COMMAND_CAP,
        L"\"%ls\" -NoLogo -NoProfile -ExecutionPolicy Bypass -STA -File \"%ls\"",
        powershell, script) < 0)
        return Fail(L"The game folder path is too long. Extract the ZIP into a shorter location such as Downloads.");
    /* The script signals after its GUI dependencies initialize. If parsing or
     * startup fails earlier, its hidden console cannot explain the failure. */
    swprintf(readyName, 80, L"Local\\ZwebtiMixLaunch-%lu", GetCurrentProcessId());
    readyEvent = CreateEventW(NULL, TRUE, FALSE, readyName);
    if (readyEvent) SetEnvironmentVariableW(L"ZWEBTI_LAUNCHER_READY_EVENT", readyName);
    startup.cb = sizeof(startup);
    if (!CreateProcessW(powershell, command, NULL, NULL, FALSE, CREATE_NO_WINDOW,
            NULL, packageRoot, &startup, &process)) {
        if (readyEvent) CloseHandle(readyEvent);
        return Fail(L"Kingdom Hearts Chain of Memories: Zwebti Mix could not start its launcher. Open START_HERE.html for help, or try PLAY.cmd from the extracted folder. No game files were changed.");
    }
    CloseHandle(process.hThread);
    result = WaitForSingleObject(process.hProcess, INFINITE);
    if (result != WAIT_OBJECT_0 || !GetExitCodeProcess(process.hProcess, &result)) {
        CloseHandle(process.hProcess);
        if (readyEvent) CloseHandle(readyEvent);
        return Fail(L"Windows could not read the launch result. Open START_HERE.html for help.");
    }
    CloseHandle(process.hProcess);
    if (readyEvent) {
        int started = WaitForSingleObject(readyEvent, 0) == WAIT_OBJECT_0;
        CloseHandle(readyEvent);
        if (result && !started)
            return Fail(L"The Windows launcher could not initialize.\n\nOpen START_HERE.html for help, or double-click PLAY.cmd to see the startup message. Keep all extracted package files together.");
    }
    return (int)result;
}
