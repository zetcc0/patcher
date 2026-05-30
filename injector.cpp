// injector.cpp
#include <windows.h>

#define GAME L"C:\\Users\\carab\\Desktop\\pinball3d\\pinball3dpatch\\Pinball.exe"


int main()
{
    WCHAR gamePath[MAX_PATH], dllPath[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, dllPath);
    wcscat_s(gamePath, GAME);
    wcscat_s(dllPath,  L"\\hook.dll");

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    CreateProcessW(gamePath, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);

    SIZE_T pathSize = (wcslen(dllPath) + 1) * sizeof(WCHAR);
    LPVOID remoteMem = VirtualAllocEx(pi.hProcess, NULL, pathSize,
                                      MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    WriteProcessMemory(pi.hProcess, remoteMem, dllPath, pathSize, NULL);

    FARPROC loadLibAddr = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");

    HANDLE hThread = CreateRemoteThread(pi.hProcess, NULL, 0,
                        (LPTHREAD_START_ROUTINE)loadLibAddr, remoteMem, 0, NULL);

    WaitForSingleObject(hThread, INFINITE);

    ResumeThread(pi.hThread);

    CloseHandle(hThread);
    VirtualFreeEx(pi.hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}