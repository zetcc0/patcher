// hook.cpp
// Compile: i686-w64-mingw32-g++ -shared -static -O2 -o hook.dll hook.cpp
#include <windows.h>
#include <TlHelp32.h>
#include "hook.h"

#define MAX_PROLOGUE 10

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

    return true;
}
