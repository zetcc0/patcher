// main.cpp
#include <windows.h>
#include <stdio.h>
#include <TlHelp32.h>
#include "hook.h"
#include <cmath>
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
    /*
    char buf[256];
    wsprintfA(buf, "Called LoadStrById with id = %d", id);
    LogToFile(buf,LSBILOGFILE);
    */
    LPSTR result = LoadStrByIdTrampoline(id, unused);

    if (id == 50) 
    {
      lstrcpynA(result, "goodppl", 256); 
    }
    else if (id == 38) 
    {
        lstrcpynA(result, "I love you Apple", 256); 
    }

    /*
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
    */
    
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

#pragma pack(push, 1)
struct RelatedToBall
{
    BYTE pad0[0x10E];
    float gravity;        // offset 0x10e
    float field_112;      // offset 0x112
    float field_116;      // offset 0x116
};

struct AutoClass2
{
    BYTE pad0[0x1E];
    RelatedToBall* related_to_ball;  // offset 0x1e
    BYTE pad1[0x40];                 // offset 0x22 -> 0x62
    float vel_x;                     // offset 0x62
    float vel_y;                     // offset 0x66
    BYTE pad3[0x4];                  // 0x6a + 0x4 = 0x6e
    float mass;                      // offset 0x6e
};
#pragma pack(pop)

static void UpdateGravityComponents(AutoClass2* self)
{
    if (!self || !self->related_to_ball) return;
    RelatedToBall* rb = self->related_to_ball;
    self->vel_x = cosf(rb->field_116) * sinf(rb->field_112) * rb->gravity;
    self->vel_y = sinf(rb->field_116) * sinf(rb->field_112) * rb->gravity;
}

AutoClass2* manager = nullptr;
void* g_infoTextBox = nullptr;
void* g_missionTextBox = nullptr;


int (__stdcall *findObjByNameTrampoline)(void* this_ptr, LPCSTR name) = nullptr;
int __stdcall findObjByName(void* this_ptr, LPCSTR name)
{
    int result = findObjByNameTrampoline(this_ptr, name);
    
    if (lstrcmpA(name, "info_text_box") == 0)
        g_infoTextBox = (void*)result;
    else if (lstrcmpA(name, "mission_text_box") == 0)
        g_missionTextBox = (void*)result;

    return result;
}

void (__stdcall *showInGameConsoleTrampoline)(void* this_ptr, const char* text, float param_2) = nullptr;
void __stdcall showInGameConsole(void* this_ptr, const char* text, float param_2)
{
    char log[512];
    sprintf(log, "showInGameConsole: text=\"%s\" param_2=%.2f", text, param_2);
    LogToFile(log, SPECIAL);
    showInGameConsoleTrampoline(this_ptr, text, param_2);
}

LRESULT  (__stdcall *MSGReceiverTrampoline)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = nullptr;
LRESULT  __stdcall MSGReceiver(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (((lParam & 0x40000000) == 0) && uMsg == 0x100 && manager != nullptr) {
        if (wParam == 'W' || wParam == VK_UP) {
            manager->related_to_ball->gravity += 5.0f;
            UpdateGravityComponents(manager);
            if (g_infoTextBox) {
                char buf[64];
                sprintf(buf, "Gravity: %.1f", manager->related_to_ball->gravity);
                showInGameConsoleTrampoline(g_infoTextBox, buf, -1.0f);
            }
        }
        if (wParam == 'S' || wParam == VK_DOWN) {
            manager->related_to_ball->gravity -= 5.0f;
            UpdateGravityComponents(manager);
            if (g_infoTextBox) {
                char buf[64];
                sprintf(buf, "Gravity: %.1f", manager->related_to_ball->gravity);
                showInGameConsoleTrampoline(g_infoTextBox, buf, -1.0f);
            }
        }
    }
    // Call the original function with all parameters
    LRESULT result = MSGReceiverTrampoline(hwnd, uMsg, wParam, lParam);
    return result;
}

void* (__stdcall  *ObjPopulatorTrampoline)(void* this_ptr, void* game_manager) = nullptr;
void* __stdcall ObjPopulator(void* this_ptr, void* game_manager)
{
    manager = (AutoClass2*)this_ptr;
    void* result = ObjPopulatorTrampoline(this_ptr, game_manager);    
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
        InstallMethodHook(0x17648, 5, (void*)&findObjByName, (void **)&findObjByNameTrampoline); // 17648
        InstallMethodHook(0x144b7, 5, (void*)&showInGameConsole, (void **)&showInGameConsoleTrampoline); // 144b7
    }
    return TRUE;
}