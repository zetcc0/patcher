// hook.cpp
// Compile: i686-w64-mingw32-g++ -shared -static -O2 -o hook.dll hook.cpp
#include <windows.h>
#include <stdio.h>
#include <TlHelp32.h>

#define LOGFILE "C:\\Users\\carab\\Desktop\\pinball3d\\patcher\\hook_calls.txt"
#define MAX_PROLOGUE 10

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
func_t Trampoline = nullptr;

LPSTR __stdcall Hook(short id, DWORD unused)
{
    char buf[256];
    wsprintfA(buf, "Called with id = %d", id);
    LogToFile(buf);
    LPSTR result = Trampoline(id, unused);

    if (id == 50) 
    {
      lstrcpynA(result, "boringggg", 256); 
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
bool InstallHook(DWORD offset, int prologeBytes, void* targetHook, void** outTrampoline)
{
    DWORD hooked_func_addr = (DWORD)GetModuleHandleA(NULL) + offset;
    BYTE orig[MAX_PROLOGUE];
    memcpy(orig, (void*)hooked_func_addr, prologeBytes);

    void* tramp = VirtualAlloc(0, prologeBytes + 5, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!tramp) { return false; }
    
    memcpy(tramp, orig, prologeBytes);
    WriteJmp((BYTE*)tramp+prologeBytes, (void*)(hooked_func_addr+prologeBytes));

    *outTrampoline = tramp;

    FreezeThreads(TRUE);
    DWORD old_protect;
    VirtualProtect((void*)hooked_func_addr, prologeBytes, PAGE_EXECUTE_READWRITE, &old_protect);
    WriteJmp((void*)hooked_func_addr, targetHook);
    VirtualProtect((void*)hooked_func_addr, prologeBytes, old_protect, &old_protect);
    FlushInstructionCache(GetCurrentProcess(), (void*)hooked_func_addr, prologeBytes); // update cpu's cache with the new written code
    FreezeThreads(FALSE);

    LogToFile("Hook installed.");

    return true;
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