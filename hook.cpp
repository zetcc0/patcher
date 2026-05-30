// hook.cpp
// Compile: i686-w64-mingw32-g++ -shared -static -O2 -o hook.dll hook.cpp
#include <windows.h>
#include <stdio.h>
#include <TlHelp32.h>

#define LOGFILE "C:\\Users\\carab\\Desktop\\pinball3d\\patcher\\hook_calls.txt"

// ---------- thread‑safe logging ----------
static CRITICAL_SECTION cs;          // guards file access

static void LogToFile(const char* msg)
{
    EnterCriticalSection(&cs);
    FILE* f = fopen(LOGFILE, "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
    LeaveCriticalSection(&cs);
}

typedef LPSTR (__stdcall *func_t)(short, DWORD);
func_t Original = nullptr;

LPSTR __stdcall Hook(short id, DWORD unused)
{
    char buf[256];
    wsprintfA(buf, "Called with id = %d", id);
    LogToFile(buf);
    LPSTR result = Original(id, unused);

    if (id == 50) 
    {
      lstrcpynA(result, "Boring", 256); 
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

// ---------- write a relative JMP ----------
void WriteJmp(void* from, void* to)
{
    DWORD rel = (DWORD)to - (DWORD)from - 5;
    BYTE* p = (BYTE*)from;
    p[0] = 0xE9;
    memcpy(p+1, &rel, 4);
}

// ---------- freeze / thaw all other threads ----------
void FreezeThreads(BOOL freeze)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    THREADENTRY32 te = { sizeof(te) };
    DWORD cur = GetCurrentThreadId(), pid = GetCurrentProcessId();
    if (Thread32First(snap, &te)) do {
        if (te.th32OwnerProcessID == pid && te.th32ThreadID != cur)
        {
            HANDLE h = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
            if (h) { freeze ? SuspendThread(h) : ResumeThread(h); CloseHandle(h); }
        }
    } while (Thread32Next(snap, &te));
    CloseHandle(snap);
}

// ---------- install the hook ----------
void Install()
{
    DWORD addr = (DWORD)GetModuleHandleA(NULL) + 0x3752;
    BYTE orig[5];
    memcpy(orig, (void*)addr, 5);

    void* tramp = VirtualAlloc(0, 10, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    memcpy(tramp, orig, 5);
    WriteJmp((BYTE*)tramp+5, (void*)(addr+5));
    Original = (func_t)tramp;

    FreezeThreads(TRUE);
    DWORD old;
    VirtualProtect((void*)addr, 5, PAGE_EXECUTE_READWRITE, &old);
    WriteJmp((void*)addr, (void*)&Hook);
    VirtualProtect((void*)addr, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), (void*)addr, 5);
    FreezeThreads(FALSE);

    LogToFile("Hook installed.");
}

// ---------- DLL entry point ----------
BOOL WINAPI DllMain(HINSTANCE h, DWORD r, LPVOID)
{
    if (r == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(h);
        InitializeCriticalSection(&cs);
        DeleteFileA(LOGFILE);
        Install();
    }
    return TRUE;
}