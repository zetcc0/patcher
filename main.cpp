// main.cpp
#include <windows.h>
#include <stdio.h>
#include <TlHelp32.h>
#include "hook.h"
#define LOGFILE "C:\\Users\\carab\\Desktop\\pinball3d\\patcher\\hook_calls.txt"


static CRITICAL_SECTION cs;          // guards file access
static void LogToFile(const char* msg)
{
    EnterCriticalSection(&cs);
    FILE* f = fopen(LOGFILE, "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
    LeaveCriticalSection(&cs);
}

typedef LPSTR (__stdcall *func_t)(short, DWORD);
func_t Trampoline = nullptr;

LPSTR __stdcall Hook(short id, DWORD unused)
{
    char buf[256];
    wsprintfA(buf, "Called with id = %d", id);
    LogToFile(buf);
    LPSTR result = Trampoline(id, unused);

    if (id == 50) 
    {
      lstrcpynA(result, "boringgggg", 256); 
    }
    else if (id == 38) 
    {
        lstrcpynA(result, "I love you Apple", 256); 
    }

    if (result)
    {
        char copy[512];
        char log[512];
        strncpy(copy, result, 511);
        copy[511] = '\0';
        wsprintfA(log, "Returned: \"%s\"", copy);
        LogToFile(log);
    }
    else { LogToFile("Returned NULL"); }

    
    return result;
}

// ---------- DLL entry point ----------
BOOL WINAPI DllMain(HINSTANCE h, DWORD r, LPVOID)
{
    if (r == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(h);
        InitializeCriticalSection(&cs);
        DeleteFileA(LOGFILE);
        InstallHook(0x3752, 5, (void*)&Hook, (void**)&Trampoline);
    }
    return TRUE;
}