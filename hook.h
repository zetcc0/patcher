// hook.h
#ifndef HOOK_H
#define HOOK_H
#include <windows.h>
bool InstallHook(DWORD offset, int prologeBytes, void* targetHook, void** outTrampoline);

#endif