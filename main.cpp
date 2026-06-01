// main.cpp
#include <windows.h>
#include <stdio.h>
#include <TlHelp32.h>
#include "hook.h"
#define LSBILOGFILE "C:\\Users\\carab\\Desktop\\pinball3d\\patcher\\lsbilf.txt"
#define PLOGFILE "C:\\Users\\carab\\Desktop\\pinball3d\\patcher\\plf.txt"
#define MSGLOGFILE "C:\\Users\\carab\\Desktop\\pinball3d\\patcher\\MSG.txt"
#define METHODLOGFILE "C:\\Users\\carab\\Desktop\\pinball3d\\patcher\\method.txt"
#define SPECIAL "C:\\Users\\carab\\Desktop\\pinball3d\\patcher\\SPECIAL.txt"

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
      lstrcpynA(result, "goodppl", 256); 
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

LRESULT  (__stdcall *MSGReceiverTrampoline)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = nullptr;
LRESULT  __stdcall MSGReceiver(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    char buf[256];
    
    if (uMsg == 0x102) {
        wsprintfA(buf, "Called message receiver with uMsg=0x%X, wParam=%c, lParam=0x%X", uMsg, wParam, lParam);
    } else {
        wsprintfA(buf, "Called message receiver with uMsg=0x%X, wParam=0x%X, lParam=0x%X", uMsg, wParam, lParam);
    }
    LogToFile(buf, MSGLOGFILE);
    if (wParam == 0xbf && ((lParam & 0x40000000) == 0) && uMsg == 0x100) {
        LogToFile("s goodpplS", SPECIAL);
    }
    // Call the original function with all parameters
    LRESULT result = MSGReceiverTrampoline(hwnd, uMsg, wParam, lParam);


    //char log[512];
    //wsprintfA(log, "Returned: %u (0x%08X)", result, result);
    //ogToFile(log, MSGLOGFILE);

    return result;
}


void* (__stdcall  *ObjPopulatorTrampoline)(void* this_ptr, void* game_manager) = nullptr;

void* __stdcall  ObjPopulator(void* this_ptr, void* game_manager)
{
    LogToFile("start", METHODLOGFILE);
    void* result = ObjPopulatorTrampoline(this_ptr, game_manager); // just works
    LogToFile("end", METHODLOGFILE);
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
        DeleteFileA(MSGLOGFILE);
        DeleteFileA(SPECIAL);
        DeleteFileA(METHODLOGFILE);
        InstallHook(0x3752, 5, (void*)&LoadStrById, (void**)&LoadStrByIdTrampoline);
        InstallHook(0x14bf9, 5, (void*)&physics, (void**)&physicsTrampoline); //14bf9
        InstallHook(0x7a3e, 5, (void *)&MSGReceiver, (void**)&MSGReceiverTrampoline); 
        InstallMethodHook(0x1a5ab, 5, (void*)&ObjPopulator, (void **)&ObjPopulatorTrampoline); // 1a5ab
    }
    return TRUE;
}