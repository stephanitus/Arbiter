#include <stdio.h>
#include <windows.h>

// NONFUNCTIONAL (obviously)

#define BUFFER_SIZE 512

int main(){
    // Create named pipe for this implant
    HANDLE server;
    server = CreateNamedPipe(
        "\\\\.\\pipe\\crack",                                                               // pipe name
        PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,                                 // pipe open opts
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_ACCEPT_REMOTE_CLIENTS, // pipe type
        1,                                                                                  // max pipe instances
        BUFFER_SIZE,                                                                        // out buffer size
        BUFFER_SIZE,                                                                        // in buffer size
        0,                                                                                  // timeout value, 0 = default 50ms'
        NULL                                                                                // default security attribute
        );

    if(server == INVALID_HANDLE_VALUE){
        printf("CreateNamedPipe failure, GLE=%d.\n", GetLastError());
        return -1;
    }

    // Connect to this client's named pipe
    HANDLE client;
    client = CreateFile(
        "\\\\.\\pipe\\crack", // lpFileName
        GENERIC_READ | GENERIC_WRITE, // dwDesiredAccess
        , // dwShareMode
        , // lpSecurityAttributes (optional)
        , // dwCreationDisposition
        , // dwFlagsAndAttributes
         // htemplateFile (optional)
        );

    return 0;
}