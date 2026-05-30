// main.cpp
#include <windows.h>
#include <stdio.h>
#include <TlHelp32.h>
#include "hook.h"
#define LSBILOGFILE "C:\\Users\\carab\\Desktop\\pinball3d\\patcher\\lsbilf.txt"
#define PLOGFILE "C:\\Users\\carab\\Desktop\\pinball3d\\patcher\\plf.txt"


static CRITICAL_SECTION cs;          // guards file access
static void LogToFile(const char* msg, LPCSTR path)
{
    EnterCriticalSection(&cs);
    FILE* f = fopen(path, "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
    LeaveCriticalSection(&cs);
}

LPSTR (__stdcall *LoadStrByIdTrampoline)(short, DWORD) = nullptr;
LPSTR __stdcall LoadStrById(short id, DWORD unused)
{
    char buf[256];
    wsprintfA(buf, "Called LoadStrById with id = %d", id);
    LogToFile(buf,LSBILOGFILE);
    LPSTR result = LoadStrByIdTrampoline(id, unused);

    if (id == 50) 
    {
      lstrcpynA(result, "boringg", 256); 
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
        LogToFile(log, LSBILOGFILE);
    }
    else { LogToFile("Returned NULL", LSBILOGFILE); }

    
    return result;
}


int (__stdcall *physicsTrampoline)(int) = nullptr;
int __stdcall physics(int id)
{
    char buf[256];
    wsprintfA(buf, "Called physics with id = %d", id);
    LogToFile(buf, PLOGFILE);
    int result = physicsTrampoline(id);

    if (result)
    {
        char log[512];
        wsprintfA(log, "Returned: \"%d\"", result);
        LogToFile(log, PLOGFILE);
    }
    else { LogToFile("Returned NULL", PLOGFILE); }

    
    return result;
}


// ---------- DLL entry point ----------
BOOL WINAPI DllMain(HINSTANCE h, DWORD r, LPVOID)
{
    if (r == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(h);
        InitializeCriticalSection(&cs);
        DeleteFileA(LSBILOGFILE);
        DeleteFileA(PLOGFILE);
        InstallHook(0x3752, 5, (void*)&LoadStrById, (void**)&LoadStrByIdTrampoline);
        InstallHook(0x14bf9, 5, (void*)&physics, (void**)&physicsTrampoline); //14bf9

    }
    return TRUE;
}