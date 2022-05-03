/*
Collection of situational awareness functions
*/

#ifndef INVESTIGATE_H
#define INVESTIGATE_H

#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <dirent.h>
#include <iphlpapi.h>
#include <string>

#endif



// Use iphlpapi to get interface info
void NetworkInterfaces(){

}

// List environment variables
void EnvVariables(){

}

// Print all running processes
void RunningProcesses(){
    HANDLE hSnap;
    PROCESSENTRY32 ProcessStruct;
    ProcessStruct.dwSize = sizeof(PROCESSENTRY32);
    hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(hSnap == INVALID_HANDLE_VALUE){
        printf("Snapshot failure: %d\n", GetLastError());
    }
    if(Process32First(hSnap, &ProcessStruct) == FALSE){
        printf("Snapshot failure: %d\n", GetLastError());
    }
    do{
        printf("%s\n", ProcessStruct.szExeFile);
    }while(Process32Next(hSnap, &ProcessStruct));
    CloseHandle(hSnap);
}

// Identify system using product ID
wchar_t* ProductID(){
    wchar_t* value = (wchar_t*)malloc(30*sizeof(wchar_t));
	DWORD size = 30*sizeof(wchar_t);
	RegGetValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), TEXT("ProductId"), RRF_RT_REG_SZ, NULL, (PVOID)value, &size);
    return value;
}

// Identify system using product ID
std::wstring MachineGUID(){
    wchar_t* value = (wchar_t*)malloc(255*sizeof(wchar_t));
	DWORD size = 255*sizeof(wchar_t);
	RegGetValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Cryptography"), TEXT("MachineGuid"), RRF_RT_REG_SZ, NULL, (PVOID)value, &size);
    return std::wstring(value, (int)size);
}

// Retrieve NETBios computer name
std::string ComputerName(){
    char* value = (char*)malloc(30*sizeof(wchar_t));;
	DWORD size = 30*sizeof(char);
	RegGetValue(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName"), TEXT("ComputerName"), RRF_RT_REG_SZ, NULL, (PVOID)value, &size);
	return std::string(value);
}

// Retrieve current user
wchar_t* CurrentUser(){
    wchar_t* value = (wchar_t*)malloc(30*sizeof(wchar_t));;
	DWORD size = 30*sizeof(char);
	RegGetValue(HKEY_CURRENT_USER, TEXT("Volatile Environment"), TEXT("USERNAME"), RRF_RT_REG_SZ, NULL, (PVOID)value, &size);
	return value;
}

// Windows release
wchar_t* OSName(){
    wchar_t* name = (wchar_t*)malloc(50*sizeof(wchar_t));
    DWORD size = 50*sizeof(char);
    RegGetValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), TEXT("ProductName"), RRF_RT_REG_SZ, NULL, (PVOID)name, &size);
    return name;
}

// Windows build
wchar_t* OSVersion(){
    wchar_t* build = (wchar_t*)malloc(30*sizeof(wchar_t));
	DWORD size = 30*sizeof(char);
	RegGetValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), TEXT("CurrentBuild"), RRF_RT_REG_SZ, NULL, (PVOID)build, &size);
    return build;
}

// List files in directory
void ls(char* path){
    // Open context
    DIR *d;
    d = opendir(path);

    struct dirent *de;
    if (d != NULL){
        while ( de = readdir(d)){
            printf("%s\n", de->d_name);
        }
        closedir(d);
    }
}